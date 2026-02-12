#include "Resources.hpp"
#include "Common.hpp"
#include "CategoryCasts.hpp"
#include "ContainerUtils.hpp"
#include "CoroCore.hpp"
#include "Coroutines.hpp"
#include "GLAPIBinding.hpp"
#include "GLBuffers.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLTextures.hpp"
#include "LODPack.hpp"
#include "MallocSupport.hpp"
#include "ResourceFiles.hpp"
#include "MeshRegistry.hpp"
#include "MeshStorage.hpp"
#include "Ranges.hpp"
#include "Resource.hpp"
#include "ResourceFiles.hpp"
#include "ResourceLoader.hpp"
#include "Errors.hpp"
#include "Scalars.hpp"
#include "SkeletalAnimation.hpp"
#include "UUID.hpp"
#include "VertexFormats.hpp"
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

struct LODRange
{
    u8 beg_lod;
    u8 end_lod;
};

auto next_lod_range(u8 cur_lod, u8 num_lods)
    -> LODRange
{
    // TODO: Something more advanced...
    assert(cur_lod);
    const u8 lod = std::max(cur_lod, u8(1)) - 1;
    const u8 end = lod + 1;
    return { lod, end };
}

struct StagingBuffers
{
    UniqueUntypedBuffer verts;
    UniqueBuffer<u32>   elems;
};

auto stage_lod(
    Span<const ubyte> verts_bytes,
    Span<const ubyte> elems_bytes)
        -> StagingBuffers
{
    const StoragePolicies policies = {
        .mode        = StorageMode::StaticServer,
        .mapping     = PermittedMapping::NoMapping,
        .persistence = PermittedPersistence::NotPersistent,
    };

    UniqueBuffer<u32>   dst_elems;
    UniqueUntypedBuffer dst_verts;

    dst_elems->specify_storage(pun_span<const u32>(elems_bytes), policies);
    dst_verts->as_typed<ubyte>().specify_storage(verts_bytes, policies);

    return { MOVE(dst_verts), MOVE(dst_elems) };
};

template<typename VertexT>
auto upload_lods(
    MeshStorage<VertexT>&        storage,
    LODPack<MeshID<VertexT>, 8>& lod_pack,
    std::ranges::range auto&&    lod_ids,
    Span<const StagingBuffers>   staged_lods)
{
    for (const auto [i, staged] : zip(lod_ids, staged_lods))
    {
        glapi::make_available<Binding::ArrayBuffer>       (staged.verts->id());
        glapi::make_available<Binding::ElementArrayBuffer>(staged.elems->id());
        lod_pack.lods[i] = storage.insert_buffer(staged.verts->template as_typed<VertexT>(), staged.elems);
    }
};

} // namespace

auto load_static_mesh(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try
{
    co_await reschedule_to(context.thread_pool());

    auto file = StaticMeshFile::open(context.resource_database().map_resource(uuid));
    const auto& header = file.header();

    // FIXME: Failure after creating the first epoch will probably break the
    // resource registry. And I forgot why. Was it because partial loads cannot
    // be cancelled? Maybe we should figure out a way to communicate that properly instead?

    ResourceProgress progress = ResourceProgress::Incomplete;
    ResourceUsage    usage{};

    StaticVector<StagingBuffers,  8> staged_lods;
    LODPack<MeshID<VertexStatic>, 8> lod_pack{};

    const u8 num_lods   = header.num_lods;
    u8       cur_lod    = num_lods;
    bool     first_time = true;
    do
    {
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
        for (const auto lod_id : lod_ids)
            staged_lods.emplace_back(stage_lod(file.lod_verts_bytes(lod_id), file.lod_elems_bytes(lod_id)));

        // Wait until this lod is staged then go to the main context.
        co_await context.completion_context().until_ready_on(context.offscreen_context(), create_fence());
        co_await reschedule_to(context.local_context());

        upload_lods(context.mesh_registry().ensure_storage_for<VertexStatic>(), lod_pack, lod_ids, staged_lods);

        // Fence the upload from the main context, await in the offscreen.
        // TODO: Does this need to flush? What if it auto-flushes on fence creation?
        // That would actually be even worse. We probably want to avoid that...

        // FIXME: Do we need a fence here at all?
        co_await context.completion_context().until_ready_on(context.offscreen_context(), create_fence());
        co_await reschedule_to(context.thread_pool());

        if (beg_lod == 0) progress = ResourceProgress::Complete;

        if (first_time)
        {
            first_time = false;
            usage = context.create_resource<RT::StaticMesh>(uuid, progress, StaticMeshResource{
                .lods = lod_pack,
                .aabb = header.aabb,
            });
        }
        else
        {
            context.update_resource<RT::StaticMesh>(uuid, [&](StaticMeshResource& mesh)
                -> ResourceProgress
            {
                // TODO: Uhh, is this right? Is this how we update this?
                mesh.lods = lod_pack;
                return progress;
            });
        }

        cur_lod = beg_lod;
    }
    while (cur_lod != 0);
}
catch (...)
{
    context.fail_resource<RT::StaticMesh>(uuid);
}

auto load_skinned_mesh(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try
{
    co_await reschedule_to(context.thread_pool());

    auto file = SkinnedMeshFile::open(context.resource_database().map_resource(uuid));
    const auto& header = file.header();

    ResourceProgress progress = ResourceProgress::Incomplete;
    ResourceUsage    usage{};

    StaticVector<StagingBuffers,   8> staged_lods;
    LODPack<MeshID<VertexSkinned>, 8> lod_pack{};

    const u8 num_lods   = header.num_lods;
    u8       cur_lod    = num_lods;
    bool     first_time = true;
    do
    {
        co_await reschedule_to(context.offscreen_context());

        staged_lods.clear();
        const auto [beg_lod, end_lod] = next_lod_range(cur_lod, num_lods);
        const auto lod_ids            = reverse(irange(beg_lod, end_lod));
        for (const auto lod_id : lod_ids)
            staged_lods.emplace_back(stage_lod(file.lod_verts_bytes(lod_id), file.lod_elems_bytes(lod_id)));

        co_await context.completion_context().until_ready_on(context.offscreen_context(), create_fence());
        co_await reschedule_to(context.local_context());

        upload_lods(context.mesh_registry().ensure_storage_for<VertexSkinned>(), lod_pack, lod_ids, staged_lods);

        co_await context.completion_context().until_ready_on(context.offscreen_context(), create_fence());
        co_await reschedule_to(context.thread_pool());

        if (beg_lod == 0) progress = ResourceProgress::Complete;

        if (first_time)
        {
            first_time = false;
            usage = context.create_resource<RT::SkinnedMesh>(uuid, progress, SkinnedMeshResource{
                .lods          = lod_pack,
                .aabb          = header.aabb,
                // NOTE: The unpacking side should request the load of the skeleton.
                // TODO: Unfortunately we currently have no way to start loading the skeleton
                // before the first LOD arrives. This might be fixed by adding another "epoch"
                // but then the unpacking side needs to understand that the first update
                // might not make any new LODs available, only the skeleton UUID.
                .skeleton_uuid = header.skeleton_uuid,
            });
        }
        else
        {
            context.update_resource<RT::SkinnedMesh>(uuid, [&](SkinnedMeshResource& mesh)
                -> ResourceProgress
            {
                mesh.lods = lod_pack;
                return progress;
            });
        }

        cur_lod = beg_lod;
    }
    while (cur_lod != 0);
}
catch (...)
{
    context.fail_resource<RT::SkinnedMesh>(uuid);
}

auto load_mdesc(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try
{
    co_await reschedule_to(context.thread_pool());

    auto mregion = context.resource_database().map_resource(uuid);
    auto text    = to_span<char>(mregion);
    const json j = json::parse(text.begin(), text.end());

    // NOTE: We are not loading the dependencies here. This is a bit odd.
    auto _ = context.create_resource<RT::MeshDesc>(uuid, ResourceProgress::Complete, MeshDescResource{
        .mesh_uuid     = deserialize_uuid(j.at("mesh")    .as_string_view()),
        .material_uuid = deserialize_uuid(j.at("material").as_string_view()),
    });
}
catch(...)
{
    context.fail_resource<RT::MeshDesc>(uuid);
}

auto load_material(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try
{
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
}
catch (...)
{
    context.fail_resource<RT::Material>(uuid);
}

namespace {

using FileEncoding   = TextureFile::Encoding;
using FileColorspace = TextureFile::Colorspace;

auto pick_internal_format(FileColorspace colorspace, usize num_channels) noexcept
    -> InternalFormat
{
    switch (colorspace)
    {
        using enum FileColorspace;
        case Linear:
            switch (num_channels)
            {
                case 1: return InternalFormat::R8;
                case 2: return InternalFormat::RG8;
                case 3: return InternalFormat::RGB8;
                case 4: return InternalFormat::RGBA8;
                default: break;
            } break;
        case sRGB:
            switch (num_channels)
            {
                case 3: return InternalFormat::SRGB8;
                case 4: return InternalFormat::SRGBA8;
                default: break;
            } break;
    }
    panic("Invalid image parameters.");
}

auto pick_pixel_data_format(FileEncoding encoding, usize num_channels) noexcept
    -> PixelDataFormat
{
    using enum FileEncoding;
    if (num_channels == 3) return PixelDataFormat::RGB;
    if (num_channels == 4) return PixelDataFormat::RGBA;
    panic();
}

auto needs_decoding(FileEncoding encoding)
    -> bool
{
    switch (encoding)
    {
        using enum FileEncoding;
        case PNG:
            return true;
        case RAW:
        case BC7:
        default:
            return false;
    }
}

// TODO: Maybe we could aready write these helpers once and not torture ourselves
// recreating this every time this information is needed in 300 different places.
auto expected_size(Extent2I resolution, usize num_channels, PixelDataType type)
    -> usize
{
    const usize num_pixels = usize(resolution.width) * usize(resolution.height);
    const usize channel_size = eval%[&]{
        switch (type)
        {
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
    };
    return num_pixels * num_channels * channel_size;
}

struct DecodedImage
{
    unique_malloc_ptr<ubyte[]> bytes;
    usize                      size_bytes;
    auto span() const noexcept -> Span<ubyte> { return { bytes.get(), size_bytes }; }
};

auto decode_texture_async_png(
    ResourceLoaderContext& context,
    Span<const ubyte>      bytes,
    usize                  num_channels)
        -> Job<DecodedImage>
{
    co_await reschedule_to(context.thread_pool());

    auto      ctx_owner = detail::make_spng_decoding_context();
    spng_ctx* ctx       = ctx_owner.get();
    int       err       = 0;

    err = spng_set_png_buffer(ctx, bytes.data(), bytes.size_bytes());
    if (err) throw_fmt("Failed setting PNG buffer: {}.", spng_strerror(err));

    const auto format = eval%[&]{
        switch (num_channels)
        {
            case 3: return SPNG_FMT_RGB8;
            case 4: return SPNG_FMT_RGBA8;
            default: panic();
        }
    };

    usize decoded_size;
    err = spng_decoded_image_size(ctx, format, &decoded_size);
    if (err) throw_fmt("Failed querying PNG image size: {}.", spng_strerror(err));

    auto decoded_bytes = malloc_unique<ubyte[]>(decoded_size);
    err = spng_decode_image(ctx, decoded_bytes.get(), decoded_size, format, 0);
    if (err) throw_fmt("Failed decoding PNG image: {}.", spng_strerror(err));

    co_return DecodedImage{
        .bytes      = MOVE(decoded_bytes),
        .size_bytes = decoded_size,
    };
}

auto decode_and_upload_mip(
    ResourceLoaderContext& context,
    const TextureFile&     file,
    RawTexture2D<>         texture,
    u8                     mip_id)
        -> Job<>
{
    const auto&         header       = file.header();
    const auto&         mip          = file.mip_span(mip_id);
    const usize         num_channels = header.num_channels;
    const PixelDataType type         = PixelDataType::UByte;

    const FileEncoding      src_encoding = mip.encoding;
    const PixelDataFormat   format       = pick_pixel_data_format(src_encoding, num_channels);
    const MipLevel          level        = int(mip_id);
    const Extent2I          resolution   = Extent2I(mip.width, mip.height);
    const Span<const ubyte> src_bytes    = file.mip_bytes(mip_id);

    assert(needs_decoding(src_encoding));

    DecodedImage decoded_image =
        co_await decode_texture_async_png(context, src_bytes, num_channels);

    if (expected_size(resolution, num_channels, type) != decoded_image.size_bytes)
        throw RuntimeError("Size does not match resolution.");

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
    u8                     mip_id)
        -> Job<>
{
    const auto&         header       = file.header();
    const auto&         mip          = file.mip_span(mip_id);
    const usize         num_channels = header.num_channels;
    const PixelDataType type         = PixelDataType::UByte;

    // TODO: Handle BC7 properly.

    const FileEncoding     src_encoding = mip.encoding;
    const PixelDataFormat  format       = pick_pixel_data_format(src_encoding, num_channels);
    const MipLevel         level        = int(mip_id);
    const Extent2I         resolution   = Extent2I(mip.width, mip.height);
    const Span<const byte> src_bytes    = file.mip_bytes(mip_id);

    assert(not needs_decoding(src_encoding));

    if (expected_size(resolution, num_channels, type) != src_bytes.size())
        throw RuntimeError("Size does not match resolution.");

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
try
{
    co_await reschedule_to(context.thread_pool());

    auto file = TextureFile::open(context.resource_database().map_resource(uuid));
    const auto& header = file.header();

    co_await reschedule_to(context.offscreen_context());

    SharedTexture2D texture;
    const auto           num_channels = header.num_channels;
    const FileColorspace colorspace   = header.colorspace;
    const NumLevels      num_mips     = header.num_mips;
    const auto&          mip0         = file.mip_span(0);
    const Extent2I       resolution0  = Extent2I(mip0.width, mip0.height);
    const InternalFormat iformat      = pick_internal_format(colorspace, num_channels);
    texture->allocate_storage(resolution0, iformat, num_mips);
    texture->set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);

    // - Upload MIP range
    // - Clamp MIPs
    // - Update (ask the user to not touch the other lods?)

    SmallVector<Job<>, 3> upload_jobs;

    auto usage      = ResourceUsage();
    auto progress   = ResourceProgress::Incomplete;
    u8   cur_mip    = num_mips;
    bool first_time = true;
    do
    {
        // FIXME: next_lod_range() is really dumb, and unsuitable for textures.
        const auto [beg_mip, end_mip] = next_lod_range(cur_mip, num_mips);
        const auto mip_ids            = reverse(irange(beg_mip, end_mip));
        cur_mip = beg_mip;

        // Upload data for new mips.
        upload_jobs.clear();
        for (const auto mip_id : mip_ids)
        {
            const auto encoding = file.mip_span(mip_id).encoding;

            if (needs_decoding(encoding))
                upload_jobs.emplace_back(decode_and_upload_mip(context, file, texture, mip_id));
            else
                upload_jobs.emplace_back(upload_mip(context, file, texture, mip_id));
        }

        // NOTE: All uploading jobs are finishing in the offscreen
        // context, so the last one will resume there too.
        // TODO: Ready or succeed? Do we care? How can it fail anyway?
        co_await until_all_succeed(upload_jobs);

        // NOTE: Only fencing after uploading multiple MIPs in a batch.
        co_await context.completion_context().until_ready_on(context.offscreen_context(), create_fence());

        if (cur_mip == 0) progress = ResourceProgress::Complete;

        if (first_time)
        {
            first_time = false;
            // Clamp available MIP region.
            // NOTE: This will explode if not done from the GPU context.
            texture->set_base_level(cur_mip);
            usage = context.create_resource<RT::Texture>(uuid, progress, TextureResource{
                .texture = texture
            });
        }
        else
        {
            context.update_resource<RT::Texture>(uuid, [&](TextureResource& resource)
                -> ResourceProgress
            {
                texture->set_base_level(cur_mip);
                return progress; // This is very awkward.
            });
        }
    }
    while (cur_mip != 0);
}
catch(...)
{
    context.fail_resource<RT::Texture>(uuid);
}

auto load_skeleton(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try
{
    co_await reschedule_to(context.thread_pool());

    auto file = SkeletonFile::open(context.resource_database().map_resource(uuid));

    // TODO: Not loading or storing the joint names so far.
    // That's mostly the issue with the Skeleton representation.

    auto skeleton = Skeleton{ .joints=file.joints() | ranges::to<Vector>() };

    auto _ = context.create_resource<RT::Skeleton>(uuid, ResourceProgress::Complete, SkeletonResource{
        .skeleton = std::make_shared<Skeleton>(MOVE(skeleton)),
    });
}
catch(...)
{
    context.fail_resource<RT::Skeleton>(uuid);
}

auto load_animation(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>
try
{
    co_await reschedule_to(context.thread_pool());

    auto file = AnimationFile::open(context.resource_database().map_resource(uuid));
    const auto& header = file.header();

    using JointKeyframes = AnimationClip::JointKeyframes;
    Vector<JointKeyframes> keyframes; keyframes.reserve(header.num_joints);

    // TODO: Possible to make a generic Key<TimeT, ValueT> type that converts times?
    auto kv2kv = [](const AnimationFile::KeyVec3& key) { return AnimationClip::Key<vec3>{ key.time_s, key.value }; };
    auto kq2kq = [](const AnimationFile::KeyQuat& key) { return AnimationClip::Key<quat>{ key.time_s, key.value }; };

    for (const uindex joint_id : irange(header.num_joints))
    {
        keyframes.push_back({
            .t = file.pos_keys(joint_id) | transform(kv2kv) | ranges::to<Vector>(),
            .r = file.rot_keys(joint_id) | transform(kq2kq) | ranges::to<Vector>(),
            .s = file.sca_keys(joint_id) | transform(kv2kv) | ranges::to<Vector>(),
        });
    }

    auto _ = context.create_resource<RT::Animation>(uuid, ResourceProgress::Complete, AnimationResource{
        .keyframes     = std::make_shared<Vector<JointKeyframes>>(MOVE(keyframes)),
        .duration_s    = file.header().duration_s,
        .skeleton_uuid = file.header().skeleton_uuid,
    });
}
catch (...)
{
    context.fail_resource<RT::Animation>(uuid);
}

namespace {

using Node = SceneResource::Node;
constexpr i32 no_parent = Node::no_parent;
constexpr i32 no_node   = -1;

struct NodeInfo
{
    i32 num_children = 0;
    i32 last_child   = no_node; // Last and prev instead of frist and next
    i32 prev_sibling = no_node; // so that the storage order would be preserved for siblings.
};

auto read_vec3(const json& j)
    -> vec3
{
    vec3 v{};
    if (j.size() != 3) throw RuntimeError("Vector argument must be a three element array.");
    v[0] = j[0].as<float>();
    v[1] = j[1].as<float>();
    v[2] = j[2].as<float>();
    return v;
}

auto read_quat(const json& j)
    -> quat
{
    quat q{};
    if (j.size() != 4) throw RuntimeError("Quaternion argument must be a four element array.");
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
    -> i32
{
    return j.get_value_or<i32>("parent", no_parent);
}

void populate_nodes_preorder(
    Vector<Node>&        dst_nodes,
    i32                  dst_parent_idx,
    i32                  src_current_idx,
    Span<const NodeInfo> infos,
    const json&          entities_array)
{
    const i32   dst_current_idx = i32(dst_nodes.size());
    const auto& entity = entities_array[src_current_idx];

    dst_nodes.push_back({
        .transform    = read_transform(entity),
        .parent_index = dst_parent_idx,
        .uuid         = read_uuid(entity),
    });

    // Then iterate children.
    i32 src_child_idx = infos[src_current_idx].last_child;
    while (src_child_idx != no_node)
    {
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
try
{
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
    thread_local Vector<i32>      roots; roots.clear();

    for (const auto [i, entity] : enumerate(entities_array))
    {
        // Parent index in the json *source* array.
        const i32 parent_idx = read_parent_idx(entity);
        if (parent_idx == no_parent)
        {
            roots.emplace_back(i);
        }
        else
        {
            NodeInfo& parent = infos[parent_idx];
            NodeInfo& node   = infos[i];
            // NOTE: Not sure if this is correct.
            if (parent.last_child != no_node)
                node.prev_sibling = parent.last_child;

            ++parent.num_children;
            parent.last_child = i32(i);
        }
    }

    Vector<Node> nodes; nodes.reserve(entities.size());

    for (const i32 root_idx : roots)
    {
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
}
catch(...)
{
    context.fail_resource<RT::Scene>(uuid);
}


} // namespace josh
