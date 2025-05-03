#include "Coroutines.hpp"
#include "DefaultResources.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICore.hpp"
#include "GLBuffers.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
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


namespace josh {
namespace {


// TODO: Should we just change the name?
using ResourceLoaderContext = ResourceRegistry::LoaderInterface;
using boost::container::static_vector;


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
    UniqueBuffer<uint32_t> dst_elems = allocate_buffer<uint32_t>(spec.num_elems, policies);
    UniqueUntypedBuffer    dst_verts;
    dst_verts->as_typed<std::byte>().allocate_storage(spec.verts_bytes, policies);

    dst_elems->upload_data(detail::pun_span<uint32_t>(src_elems));
    dst_verts->as_typed<std::byte>().upload_data(src_verts);

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
        -> Job<void>
{
    using VertexT = VertexStatic;
    assert(file.layout() == MeshFile::VertexLayout::Static);

    ResourceProgress progress = ResourceProgress::Incomplete;
    ResourceUsage    usage{};

    static_vector<StagingBuffers, 8> staged_lods;

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
        -> Job<void>
{
    using VertexT = VertexSkinned;
    assert(file.layout() == MeshFile::VertexLayout::Skinned);

    ResourceProgress progress = ResourceProgress::Incomplete;
    ResourceUsage    usage{};

    static_vector<StagingBuffers, 8> staged_lods;

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


        cur_lod = beg_lod;
    } while (cur_lod != 0);

}


} // namespace


auto load_mesh(
    ResourceRegistry::LoaderInterface context,
    UUID                              uuid,
    MeshRegistry&                     mesh_registry)
        -> Job<void>
{
    const auto task_guard = context.task_counter().obtain_task_guard();
    co_await reschedule_to(context.thread_pool());

    MeshFile file = [&]{
        const auto on_exception = // NOTE: Scope guard only until the call to create_resource().
            boost::scope::scope_fail([&]{ context.fail_resource<RT::Mesh>(uuid); });

        mapped_region mregion = context.resource_database().map_resource(uuid);

        if (!mregion.get_address()) {
            throw error::RuntimeError(fmt::format("Failed to map resource {}.", serialize_uuid(uuid)));
        }

        return MeshFile::open(MOVE(mregion));
    }();

    // FIXME: Failure past this point will probably break the registry.

    switch (file.layout()) {
        using enum MeshFile::VertexLayout;
        case Static:
            co_await load_static_mesh(context, file, uuid, mesh_registry);
            break;
        case Skinned:
            co_await load_skinned_mesh(context, file, uuid, mesh_registry);
            break;
        default: assert(false);
    }
}


} // namespace josh
