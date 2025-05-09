#include "Asset.hpp"
#include "Common.hpp"
#include "CategoryCasts.hpp"
#include "ContainerUtils.hpp"
#include "CoroCore.hpp"
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
#include "MallocSupport.hpp"
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
#include "detail/SPNG.hpp"
#include <fmt/format.h>
#include <jsoncons/basic_json.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <spng.h>


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

    dst_elems->specify_storage(pun_span<const uint32_t>(src_elems), policies);
    dst_verts->as_typed<std::byte>().specify_storage(src_verts, policies);

    return { MOVE(dst_verts), MOVE(dst_elems) };
};

template<typename VertexT>
auto upload_lods(
    MeshStorage<VertexT>&        storage,
    LODPack<MeshID<VertexT>, 8>& lod_pack,
    std::ranges::range auto&&    lod_ids,
    Span<const StagingBuffers>   staged_lods)
{
    for (const auto [i, staged] : zip(lod_ids, staged_lods)) {
        make_available<Binding::ArrayBuffer>       (staged.verts->id());
        make_available<Binding::ElementArrayBuffer>(staged.elems->id());
        lod_pack.lods[i] = storage.insert_buffer(staged.verts->template as_typed<VertexT>(), staged.elems);
    }
};

auto load_static_mesh(
    ResourceLoaderContext& context,
    const MeshFile&        file,
    UUID                   uuid,
    MeshRegistry&          mesh_registry)
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

        // Wait until this lod is staged then go to the main context.
        co_await context.completion_context().until_ready_on(context.offscreen_context(), create_fence());
        co_await reschedule_to(context.local_context());

        upload_lods(mesh_registry.ensure_storage_for<VertexT>(), lod_pack, lod_ids, staged_lods);

        // Fence the upload from the main context, await in the offscreen.
        // TODO: Does this need to flush? What if it auto-flushes on fence creation?
        // That would actually be even worse. We probably want to avoid that...

        // FIXME: Do we need a fence here at all?
        co_await context.completion_context().until_ready_on(context.offscreen_context(), create_fence());
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
    ResourceLoaderContext& context,
    const MeshFile&        file,
    UUID                   uuid,
    MeshRegistry&          mesh_registry)
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

        co_await context.completion_context().until_ready_on(context.offscreen_context(), create_fence());
        co_await reschedule_to(context.local_context());

        upload_lods(mesh_registry.ensure_storage_for<VertexT>(), lod_pack, lod_ids, staged_lods);

        co_await context.completion_context().until_ready_on(context.offscreen_context(), create_fence());
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
    co_await reschedule_to(context.thread_pool());

    auto file = MeshFile::open(context.resource_database().map_resource(uuid));

    // FIXME: Failure past this point will probably break the registry.
    // And I forgot why. Was it because partial loads cannot be cancelled?
    // Maybe we should figure out a way to communicate that properly instead?

    auto& mesh_registry = context.mesh_registry();

    switch (file.layout()) {
        using enum MeshFile::VertexLayout;
        case Static: {
            co_await load_static_mesh(context, file, uuid, mesh_registry);
            break;
        }
        case Skinned:
            co_await load_skinned_mesh(context, file, uuid, mesh_registry);
            break;
        default: assert(false);
    }

} catch(...) {
    context.fail_resource<RT::Mesh>(uuid);
}


auto load_mdesc(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try {
    co_await reschedule_to(context.thread_pool());

    auto mregion = context.resource_database().map_resource(uuid);
    auto text    = to_span<char>(mregion);
    const json j = json::parse(text.begin(), text.end());

    // NOTE: We are not loading the dependencies here. This is a bit odd.
    auto _ = context.create_resource<RT::MeshDesc>(uuid, ResourceProgress::Complete, MeshDescResource{
        .mesh_uuid     = deserialize_uuid(j.at("mesh")    .as_string_view()),
        .material_uuid = deserialize_uuid(j.at("material").as_string_view()),
    });
} catch(...) {
    context.fail_resource<RT::MeshDesc>(uuid);
}


auto load_material(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try {
    co_await reschedule_to(context.thread_pool());

    auto mregion = context.resource_database().map_resource(uuid);
    auto text    = to_span<char>(mregion);
    const json j = json::parse(text.begin(), text.end());

    auto _ = context.create_resource<RT::Material>(uuid, ResourceProgress::Complete, MaterialResource{
        .diffuse_uuid  = deserialize_uuid(j.at("diffuse").as_string_view()),
        .normal_uuid   = deserialize_uuid(j.at("normal").as_string_view()),
        .specular_uuid = deserialize_uuid(j.at("specular").as_string_view()),
        .specpower     = j.at("specpower").as<float>(),
    });
} catch (...) {
    context.fail_resource<RT::Material>(uuid);
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
    safe_unreachable();
}

auto pick_pixel_data_format(TextureFile::StorageFormat format, size_t num_channels) noexcept
    -> PixelDataFormat
{
    using enum TextureFile::StorageFormat;
    if (num_channels == 3) {
        return PixelDataFormat::RGB;
    } else if (num_channels == 4) {
        return PixelDataFormat::RGBA;
    }
    safe_unreachable();
}

auto needs_decoding(TextureFile::StorageFormat format)
    -> bool
{
    switch (format) {
        using enum TextureFile::StorageFormat;
        case PNG:
            return true;
        case RAW:
        case BC7:
        case _count:
        default:
            return false;
    }
}

// TODO: Maybe we could aready write these helpers once and not torture ourselves
// recreating this every time this information is needed in 300 different places.
auto expected_size(Extent2I resolution, size_t num_channels, PixelDataType type)
    -> size_t
{
    const size_t num_pixels = size_t(resolution.width) * size_t(resolution.height);
    const size_t channel_size = [&]{
        switch (type) {
            using PDT = PixelDataType;
            case PDT::UByte:
            case PDT::Byte:
                return 1;
            case PDT::Short:
            case PDT::UShort:
            case PDT::HalfFloat:
                return 2;
            case PDT::Int:
            case PDT::UInt:
            case PDT::Float:
                return 4;
            default:
                throw RuntimeError("PixelDataType not supported.");
        }
    }();
    return num_pixels * num_channels * channel_size;
}

struct DecodedImage {
    unique_malloc_ptr<byte[]> bytes;
    size_t                    size_bytes;
    auto span() const noexcept -> Span<byte> { return { bytes.get(), size_bytes }; }
};

auto decode_texture_async_png(
    ResourceLoaderContext& context,
    Span<const byte>       bytes,
    size_t                 num_channels)
        -> Job<DecodedImage>
{
    co_await reschedule_to(context.thread_pool());

    auto      ctx_owner = detail::make_spng_decoding_context();
    spng_ctx* ctx       = ctx_owner.get();
    int       err       = 0;

    err = spng_set_png_buffer(ctx, bytes.data(), bytes.size_bytes());
    if (err) throw RuntimeError(fmt::format("Failed setting PNG buffer: {}.", spng_strerror(err)));

    const auto format = [&]{
        switch (num_channels) {
            case 3: return SPNG_FMT_RGB8;
            case 4: return SPNG_FMT_RGBA8;
            default: std::terminate();
        }
    }();

    size_t decoded_size;
    err = spng_decoded_image_size(ctx, format, &decoded_size);
    if (err) throw RuntimeError(fmt::format("Failed querying PNG image size: {}.", spng_strerror(err)));

    auto decoded_bytes = malloc_unique<byte[]>(decoded_size);
    err = spng_decode_image(ctx, decoded_bytes.get(), decoded_size, format, 0);
    if (err) throw RuntimeError(fmt::format("Failed decoding PNG image: {}.", spng_strerror(err)));

    co_return DecodedImage{
        .bytes      = MOVE(decoded_bytes),
        .size_bytes = decoded_size,
    };
}

auto decode_and_upload_mip(
    ResourceLoaderContext& context,
    const TextureFile&     file,
    RawTexture2D<>         texture,
    uint8_t                mip_id)
        -> Job<>
{
    using StorageFormat = TextureFile::StorageFormat;
    const size_t          num_channels = file.num_channels();
    const PixelDataType   type         = PixelDataType::UByte;

    const StorageFormat    src_format   = file.format(mip_id);
    const PixelDataFormat  format       = pick_pixel_data_format(src_format, num_channels);
    const MipLevel         level        = int(mip_id);
    const Extent2I         resolution   = file.resolution(mip_id);
    const Span<const byte> src_bytes    = as_bytes(file.mip_bytes(mip_id));

    assert(needs_decoding(src_format));

    DecodedImage decoded_image =
        co_await decode_texture_async_png(context, src_bytes, num_channels);

    if (expected_size(resolution, num_channels, type) != decoded_image.size_bytes) throw RuntimeError("Size does not match resolution.");

    co_await reschedule_to(context.offscreen_context());

    texture.upload_image_region(
        { {}, resolution },
        format,
        type,
        decoded_image.bytes.get(),
        level
    );
}

auto upload_mip(
    ResourceLoaderContext& context,
    const TextureFile&     file,
    RawTexture2D<>         texture,
    uint8_t                mip_id)
        -> Job<>
{
    using StorageFormat = TextureFile::StorageFormat;
    const size_t          num_channels = file.num_channels();
    const PixelDataType   type         = PixelDataType::UByte;

    // TODO: Handle BC7 properly.

    const StorageFormat    src_format   = file.format(mip_id);
    const PixelDataFormat  format       = pick_pixel_data_format(src_format, num_channels);
    const MipLevel         level        = int(mip_id);
    const Extent2I         resolution   = file.resolution(mip_id);
    const Span<const byte> src_bytes    = as_bytes(file.mip_bytes(mip_id));

    assert(not needs_decoding(src_format));

    if (expected_size(resolution, num_channels, type) != src_bytes.size()) throw RuntimeError("Size does not match resolution.");

    co_await reschedule_to(context.offscreen_context());

    texture.upload_image_region(
        { {}, resolution },
        format,
        type,
        src_bytes.data(),
        level
    );
};


} // namespace


auto load_texture(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try {
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

    SmallVector<Job<>, 3> upload_jobs;

    auto    usage      = ResourceUsage();
    auto    progress   = ResourceProgress::Incomplete;
    uint8_t cur_mip    = num_mips;
    bool    first_time = true;
    do {
        // FIXME: next_lod_range() is really dumb, and unsuitable for textures.
        const auto [beg_mip, end_mip] = next_lod_range(cur_mip, num_mips);
        const auto mip_ids            = reverse(irange(beg_mip, end_mip));
        cur_mip = beg_mip;

        // Upload data for new mips.
        upload_jobs.clear();
        for (const auto mip_id : mip_ids) {
            const auto format = file.format(mip_id);
            if (needs_decoding(format)) {
                upload_jobs.emplace_back(decode_and_upload_mip(context, file, texture, mip_id));
            } else {
                upload_jobs.emplace_back(upload_mip(context, file, texture, mip_id));
            }
        }

        // NOTE: All uploading jobs are finishing in the offscreen
        // context, so the last one will resume there too.
        // TODO: Ready or succeed? Do we care? How can it fail anyway?
        co_await until_all_succeed(upload_jobs);

        // NOTE: Only fencing after uploading multiple MIPs in a batch.
        co_await context.completion_context().until_ready_on(context.offscreen_context(), create_fence());

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


namespace {

using Node = SceneResource::Node;
constexpr int32_t no_parent  = Node::no_parent;
constexpr int32_t no_node    = -1;

struct NodeInfo {
    int32_t num_children = 0;
    int32_t last_child   = no_node; // Last and prev instead of frist and next
    int32_t prev_sibling = no_node; // so that the storage order would be preserved for siblings.
};



auto read_vec3(const json& j)
    -> vec3
{
    vec3 v{};
    if (j.size() != 3) { throw RuntimeError("Vector argument must be a three element array."); }
    v[0] = j[0].as<float>();
    v[1] = j[1].as<float>();
    v[2] = j[2].as<float>();
    return v;
}

auto read_quat(const json& j)
    -> quat
{
    quat q{};
    if (j.size() != 4) { throw RuntimeError("Quaternion argument must be a four element array."); }
    q[0] = j[0].as<float>();
    q[1] = j[1].as<float>();
    q[2] = j[2].as<float>();
    q[3] = j[3].as<float>();
    return q;
}

auto read_transform(const json& j)
    -> Transform
{
    Transform new_tf{};
    if (const auto& j_tf = j.at_or_null("transform");
        not j_tf.is_null())
    {
        if (const auto& j_pos = j_tf.at_or_null("position"); !j_pos.is_null()) { new_tf.position()    = read_vec3(j_pos); }
        if (const auto& j_rot = j_tf.at_or_null("rotation"); !j_rot.is_null()) { new_tf.orientation() = read_quat(j_rot); }
        if (const auto& j_sca = j_tf.at_or_null("scaling");  !j_sca.is_null()) { new_tf.scaling()     = read_vec3(j_sca); }
    }
    return new_tf;
}

auto read_uuid(const json& j)
    -> UUID
{
    UUID uuid{};
    if (const auto& j_uuid = j.at_or_null("uuid");
        not j_uuid.is_null())
    {
        uuid = deserialize_uuid(j_uuid.as_string_view());
    }
    return uuid;
}

auto read_parent_idx(const json& j)
    -> int32_t
{
    return j.get_value_or<int32_t>("parent", no_parent);
}

void populate_nodes_preorder(
    Vector<Node>&        dst_nodes,
    int32_t              dst_parent_idx,
    int32_t              src_current_idx,
    Span<const NodeInfo> infos,
    const json&          entities_array)
{
    const int32_t dst_current_idx = int32_t(dst_nodes.size());
    const auto& entity = entities_array[src_current_idx];

    dst_nodes.emplace_back(Node{
        .transform    = read_transform(entity),
        .parent_index = dst_parent_idx,
        .uuid         = read_uuid(entity),
    });

    // Then iterate children.
    int32_t src_child_idx = infos[src_current_idx].last_child;
    while (src_child_idx != no_node) {
        populate_nodes_preorder(
            dst_nodes,
            dst_current_idx,
            src_child_idx,
            infos,
            entities_array
        );
        src_child_idx = infos[src_child_idx].prev_sibling;
    }
}

} // namespace


auto load_scene(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try {
    co_await reschedule_to(context.thread_pool());

    auto mregion = context.resource_database().map_resource(uuid);
    auto text    = to_span<char>(mregion);
    const json j = json::parse(text.begin(), text.end());

    const auto& entities = j.at("entities");

    auto entities_array = entities.array_range();

    // Reconstruct pre-order.
    //
    // NOTE: IDK if I should even bother with this, but this is to guarantee
    // that the array is indeed stored in pre-order, which we might rely on.
    //
    // For emplacing into the scene this does not matter, but might come up
    // in other usecases.
    //
    // It is likely that we want this to be a guarantee of the internal scene
    // storage format, and not have to do this every time on load.

    // TODO: Check parent ids for being in-range.

    thread_local Vector<NodeInfo> infos; infos.resize(entities.size(), {});
    thread_local Vector<int32_t>  roots; roots.clear();

    for (const auto [i, entity] : enumerate(entities_array)) {
        // Parent index in the json *source* array.
        const int32_t parent_idx = read_parent_idx(entity);
        if (parent_idx == no_parent) {
            roots.emplace_back(i);
        } else {
            NodeInfo& parent = infos[parent_idx];
            NodeInfo& node   = infos[i];
            // NOTE: Not sure if this is correct.
            if (parent.last_child != no_node) {
                node.prev_sibling = parent.last_child;
            }
            ++parent.num_children;
            parent.last_child = int32_t(i);
        }
    }

    Vector<Node> nodes; nodes.reserve(entities.size());

    for (const int32_t root_idx : roots) {
        populate_nodes_preorder(
            nodes,
            no_parent,
            root_idx,
            infos,
            entities
        );
    }

    auto _ = context.create_resource<RT::Scene>(uuid, ResourceProgress::Complete, SceneResource{
        .nodes = std::make_shared<Vector<Node>>(MOVE(nodes)),
    });

} catch(...) {
    context.fail_resource<RT::Scene>(uuid);
}


} // namespace josh
