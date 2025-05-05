#include "Asset.hpp"
#include "Common.hpp"
#include "CategoryCasts.hpp"
#include "Coroutines.hpp"
#include "DefaultResources.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICore.hpp"
#include "GLBuffers.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLTextures.hpp"
#include "LODPack.hpp"
#include "MeshRegistry.hpp"
#include "MeshStorage.hpp"
#include "Ranges.hpp"
#include "Resource.hpp"
#include "ResourceFiles.hpp"
#include "ResourceRegistry.hpp"
#include "RuntimeError.hpp"
#include "UUID.hpp"
#include "VertexSkinned.hpp"
#include "VertexStatic.hpp"
#include "detail/AssetImporter.hpp"
#include <boost/container/static_vector.hpp>
#include <boost/scope/scope_fail.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fmt/format.h>
#include <jsoncons/basic_json.hpp>


namespace josh {
namespace {


using jsoncons::json;


struct LODRange {
    uint8_t beg_lod;
    uint8_t end_lod;
};


auto next_lod_range(uint8_t cur_lod, uint8_t num_lods)
    -> LODRange
{
    // TODO: Something more advanced...
    assert(cur_lod);
    const uint8_t lod = std::max(cur_lod, uint8_t(1)) - 1;
    const uint8_t end = lod + 1;
    return { lod, end };
}


struct StagingBuffers {
    UniqueUntypedBuffer    verts;
    UniqueBuffer<uint32_t> elems;
};


auto stage_lod(const MeshFile& file, uint8_t lod)
    -> StagingBuffers
{
    const auto spec      = file.lod_spec(lod);
    const auto src_verts = file.lod_verts_bytes(lod);
    const auto src_elems = file.lod_elems_bytes(lod);
    assert(spec.compression == MeshFile::Compression::None && "Compression not implemented.");

    const StoragePolicies policies{
        .mode        = StorageMode::StaticServer,
        .mapping     = PermittedMapping::NoMapping,
        .persistence = PermittedPersistence::NotPersistent,
    };

    UniqueBuffer<uint32_t> dst_elems;
    UniqueUntypedBuffer    dst_verts;

    dst_elems->specify_storage(detail::pun_span<const uint32_t>(src_elems), policies);
    dst_verts->as_typed<std::byte>().specify_storage(src_verts, policies);

    return { MOVE(dst_verts), MOVE(dst_elems) };
};


template<typename VertexT>
auto upload_lods(
    MeshStorage<VertexT>&           storage,
    LODPack<MeshID<VertexT>, 8>&    lod_pack,
    std::ranges::range auto&&       lod_ids,
    std::span<const StagingBuffers> staged_lods)
{
    for (const auto [i, staged] : zip(lod_ids, staged_lods)) {
        make_available<Binding::ArrayBuffer>       (staged.verts->id());
        make_available<Binding::ElementArrayBuffer>(staged.elems->id());
        lod_pack.lods[i] = storage.insert_buffer(staged.verts->template as_typed<VertexT>(), staged.elems);
    }
};


auto load_static_mesh(
    ResourceLoaderContext context,
    const MeshFile&       file,
    UUID                  uuid,
    MeshRegistry&         mesh_registry)
        -> Job<>
{
    using VertexT = VertexStatic;
    assert(file.layout() == MeshFile::VertexLayout::Static);

    ResourceProgress progress = ResourceProgress::Incomplete;
    ResourceUsage    usage{};

    StaticVector<StagingBuffers, 8> staged_lods;

    const uint8_t num_lods  = file.num_lods();
    const uint8_t first_lod = num_lods - 1;
    assert(num_lods);

    LODPack<MeshID<VertexT>, 8> lod_pack{};

    uint8_t cur_lod    = num_lods;
    bool    first_time = true;
    do {
        // FIXME: This is overall pretty bad as it waits on a previous
        // LOD to be fully inserted into the mesh storage before proceeding
        // to the next one. Each LOD could span multiple frames, and is forced
        // to span at least one.
        //
        // TODO: Could we make it possible to load LODs out-of-order? It's just
        // a small bitfield indicating availability, scanning that is very cheap.

        co_await reschedule_to(context.offscreen_context());

        staged_lods.clear();
        const auto [beg_lod, end_lod] = next_lod_range(cur_lod, num_lods);
        const auto lod_ids            = reverse(irange(beg_lod, end_lod));
        for (const auto i : lod_ids) {
            staged_lods.emplace_back(stage_lod(file, i));
        }

        // Ideally, we'd wait on a fence here.
        //   co_await context.offscreen_context().await_fence(create_fence());
        glapi::finish();


        co_await reschedule_to(context.local_context());

        upload_lods(mesh_registry.ensure_storage_for<VertexT>(), lod_pack, lod_ids, staged_lods);

        // Another fence would not hurt here.
        // (In the offscreen context, obviously).

        co_await reschedule_to(context.thread_pool());

        if (beg_lod == 0) progress = ResourceProgress::Complete;

        if (first_time) {
            first_time = false;
            usage = context.create_resource<RT::Mesh>(uuid, progress, MeshResource{
                .mesh = MeshResource::Static{ lod_pack }
            });

        } else {
            context.update_resource<RT::Mesh>(uuid, [&](MeshResource& mesh)
                -> ResourceProgress
            {
                // TODO: Uhh, is this right? Is this how we update this?
                get<MeshResource::Static>(mesh.mesh).lods = lod_pack;
                return progress;
            });
        }


        cur_lod = beg_lod;
    } while (cur_lod != 0);

}


auto load_skinned_mesh(
    ResourceLoaderContext context,
    const MeshFile&       file,
    UUID                  uuid,
    MeshRegistry&         mesh_registry)
        -> Job<>
{
    using VertexT = VertexSkinned;
    assert(file.layout() == MeshFile::VertexLayout::Skinned);

    ResourceProgress progress = ResourceProgress::Incomplete;
    ResourceUsage    usage{};

    StaticVector<StagingBuffers, 8> staged_lods;

    const uint8_t num_lods  = file.num_lods();
    const uint8_t first_lod = num_lods - 1;
    assert(num_lods);

    const auto get_skeleton = [&](UUID skeleton_uuid)
        -> Job<PrivateResource<RT::Skeleton>>
    {
        co_return co_await context.get_resource_dependency<RT::Skeleton>(skeleton_uuid);
    };

    // Launch as an async task in case the skeleton is not cached.
    auto skeleton_job = get_skeleton(file.skeleton_uuid());

    LODPack<MeshID<VertexT>, 8> lod_pack{};

    uint8_t cur_lod    = num_lods;
    bool    first_time = true;
    do {
        // FIXME: This is overall pretty bad as it waits on a previous
        // LOD to be fully inserted into the mesh storage before proceeding
        // to the next one. Each LOD could span multiple frames, and is forced
        // to span at least one.
        //
        // TODO: Could we make it possible to load LODs out-of-order? It's just
        // a small bitfield indicating availability, scanning that is very cheap.

        co_await reschedule_to(context.offscreen_context());

        staged_lods.clear();
        const auto [beg_lod, end_lod] = next_lod_range(cur_lod, num_lods);
        const auto lod_ids            = reverse(irange(beg_lod, end_lod));
        cur_lod = beg_lod;
        for (const auto i : lod_ids) {
            staged_lods.emplace_back(stage_lod(file, i));
        }

        // Ideally, we'd wait on a fence here.
        //   co_await context.offscreen_context().await_fence(create_fence());
        glapi::finish();


        co_await reschedule_to(context.local_context());

        upload_lods(mesh_registry.ensure_storage_for<VertexT>(), lod_pack, lod_ids, staged_lods);

        // Another fence would not hurt here.
        // (In the offscreen context, obviously).

        co_await reschedule_to(context.thread_pool());

        if (beg_lod == 0) progress = ResourceProgress::Complete;

        if (first_time) {
            first_time = false;
            usage = context.create_resource<RT::Mesh>(uuid, progress, MeshResource{
                .mesh = MeshResource::Skinned{
                    .lods     = lod_pack,
                    .skeleton = co_await skeleton_job,
                }
            });

        } else {
            context.update_resource<RT::Mesh>(uuid, [&](MeshResource& mesh)
                -> ResourceProgress
            {
                // TODO: Uhh, is this right? Is this how we update this?
                get<MeshResource::Skinned>(mesh.mesh).lods = lod_pack;
                return progress;
            });
        }

    } while (cur_lod != 0);

}


} // namespace


auto load_mesh(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try {
    const auto task_guard = context.task_counter().obtain_task_guard();

    co_await reschedule_to(context.thread_pool());

    auto file = MeshFile::open(context.resource_database().map_resource(uuid));

    // FIXME: Failure past this point will probably break the registry.
    // And I forgot why. Was it because partial loads cannot be cancelled?
    // Maybe we should figure out a way to communicate that properly instead?

    auto& mesh_registry = context.mesh_registry();

    switch (file.layout()) {
        using enum MeshFile::VertexLayout;
        case Static: {
            auto job = load_static_mesh(context, file, uuid, mesh_registry);
            co_await job;
            break;
        }
        case Skinned:
            co_await load_skinned_mesh(context, file, uuid, mesh_registry);
            break;
        default: assert(false);
    }

    auto i = 0;
} catch(...) {
    context.fail_resource<RT::Mesh>(uuid);
}


auto load_mdesc(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try {
    const auto task_guard = context.task_counter().obtain_task_guard();

    co_await reschedule_to(context.thread_pool());

    auto mregion = context.resource_database().map_resource(uuid);
    const json j = json::parse((char*)mregion.get_address(), (char*)mregion.get_address() + mregion.get_size());

    // NOTE: We are not loading the dependencies here. This is a bit odd.
    auto _ = context.create_resource<RT::MeshDesc>(uuid, ResourceProgress::Complete, MeshDescResource{
        .mesh_uuid     = deserialize_uuid(j.at("mesh")    .as_string_view()),
        .diffuse_uuid  = deserialize_uuid(j.at("diffuse") .as_string_view()),
        .normal_uuid   = deserialize_uuid(j.at("normal")  .as_string_view()),
        .specular_uuid = deserialize_uuid(j.at("specular").as_string_view()),
        .specpower     = j.at("specpower").as<float>(),
    });
} catch(...) {
    context.fail_resource<RT::MeshDesc>(uuid);
}


namespace {
auto pick_internal_format(ImageIntent intent, size_t num_channels) noexcept
    -> InternalFormat
{
    if (num_channels == 3) {
        return InternalFormat::SRGB8;
    } else if (num_channels == 4) {
        return InternalFormat::SRGBA8;
    }
    // TODO: other
    assert(false);
}
auto pick_pixel_data_format(TextureFile::StorageFormat format, size_t num_channels) noexcept
    -> PixelDataFormat
{
    using enum TextureFile::StorageFormat;
    // TODO: Will potentially need to before here. Uh-uh.
    assert(format == RAW && "TODO: Support other image formats.");
    if (num_channels == 3) {
        return PixelDataFormat::RGB;
    } else if (num_channels == 4) {
        return PixelDataFormat::RGBA;
    }
    assert(false);
}
} // namespace


auto load_texture(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try {
    const auto task_guard = context.task_counter().obtain_task_guard();

    co_await reschedule_to(context.thread_pool());

    auto file = TextureFile::open(context.resource_database().map_resource(uuid));

    co_await reschedule_to(context.offscreen_context());

    SharedTexture2D texture;
    const auto           num_channels = file.num_channels();
    const NumLevels      num_mips     = file.num_mips();
    const Size2I         resolution0  = file.resolution(0);
    const ImageIntent    intent       = ImageIntent::Albedo; // FIXME: Uhh, this should be in a file or something.
    const InternalFormat iformat      = pick_internal_format(intent, num_channels);
    texture->allocate_storage(resolution0, iformat, num_mips);

    // - Upload MIP range
    // - Clamp MIPs
    // - Update (ask the user to not touch the other lods?)

    auto    usage      = ResourceUsage();
    auto    progress   = ResourceProgress::Incomplete;
    uint8_t cur_mip    = num_mips;
    bool    first_time = true;
    do {
        co_await reschedule_to(context.offscreen_context());

        // FIXME: next_lod_range() is really dumb, and unsuitable for textures.
        const auto [beg_mip, end_mip] = next_lod_range(cur_mip, num_mips);
        const auto mip_ids            = reverse(irange(beg_mip, end_mip));
        cur_mip = beg_mip;

        // Upload data for new mips.
        for (const auto mip_id : mip_ids) {
            using enum TextureFile::StorageFormat;
            const auto            src_format = file.format(mip_id);
            const PixelDataType   type       = PixelDataType::UByte;
            const PixelDataFormat format     = pick_pixel_data_format(src_format, num_channels);
            const MipLevel        level      = int(mip_id);
            const Extent2I        resolution = file.resolution(mip_id);
            const auto            bytes      = file.mip_bytes(mip_id);
            texture->upload_image_region(
                { {}, resolution },
                format,
                type,
                bytes.data(),
                level
            );
        }

        // FIXME: Fence.
        glapi::finish();

        // TODO: What context do we upload this from?
        // This is potentially blocking, but it has to be a valid GL context.
        // co_await reschedule_to(context.local_context());

        if (cur_mip == 0) progress = ResourceProgress::Complete;

        if (first_time) {
            first_time = false;
            // Clamp available MIP region.
            // NOTE: This will explode if not done from the GPU context.
            texture->set_base_level(cur_mip);
            usage = context.create_resource<RT::Texture>(uuid, progress, TextureResource{
                .texture = texture
            });

        } else {
            context.update_resource<RT::Texture>(uuid, [&](TextureResource& resource)
                -> ResourceProgress
            {
                texture->set_base_level(cur_mip);
                return progress; // This is very awkward.
            });
        }

    } while (cur_mip != 0);

} catch(...) {
    context.fail_resource<RT::Texture>(uuid);
}


} // namespace josh
