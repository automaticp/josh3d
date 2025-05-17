#include "DefaultResourceFiles.hpp"
#include "Common.hpp"
#include "FileMapping.hpp"
#include "EnumUtils.hpp"
#include "CategoryCasts.hpp"
#include "Ranges.hpp"
#include "Resource.hpp"
#include "ResourceFiles.hpp"
#include "UUID.hpp"
#include <fmt/core.h>
#include <fmt/std.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <new>


namespace josh {
namespace {


template<typename DstT, typename SrcT>
auto please_type_pun(SrcT* from)
    -> DstT*
{
    // Ya-ya, that object is totally within it's lifetime.
    // In fact, I created it two months ago on another machine,
    // using a valid livetime-starting function. We're good, right?
    return std::launder(reinterpret_cast<DstT*>(from));
    // NOTE: None of the 3 compilers implement std::start_lifetime_as yet,
    // which is a proper way to do this. It's the year 2025 here, guys!
    // Can I have my sane type-punnig already?
}


auto mapping_bytes(const MappedRegion& mapping) noexcept
    -> ubyte*
{
    return (ubyte*)mapping.get_address();
}


// Convenience for single-hop access to an object at a given file offset.
template<typename T>
auto ptr_at_offset(const MappedRegion& mregion, size_t offset_bytes) noexcept
    -> T*
{
    return please_type_pun<T>(mapping_bytes(mregion) + offset_bytes);
}


template<typename HeaderT>
void throw_if_too_small_for_header(size_t file_size) noexcept(false) {
    if (file_size < sizeof(HeaderT)) {
        throw InvalidResourceFile("Resource file is too small to contain header information.");
    }
}


void throw_on_unexpected_size(size_t expected, size_t real) noexcept(false) {
    if (real != expected)
        throw InvalidResourceFile(fmt::format("Resource file unexpected size. Expected {}, got {}.", expected, real));

}

template<typename FileT>
void throw_if_mismatched_preamble(const ResourcePreamble& preamble) noexcept(false) {
    if (preamble.file_type != FileT::file_type)
        throw_fmt<InvalidResourceFile>("Mismatched file type in resource preamble. Expected {}, got {}.", FileType(FileT::file_type), preamble.file_type);
    // HMM: This only makes sense if a single file format stores only one resource type.
    if (preamble.resource_type != FileT::resource_type)
        throw_fmt<InvalidResourceFile>("Mismatched resource type in resource preamble. Expected {}, got {}.", ResourceType(FileT::resource_type), preamble.resource_type);
    // HMM: This only makes sense if there's no cross-version compatibility.
    if (preamble.version != FileT::version)
        throw_fmt<InvalidResourceFile>("Mismatched version in resource preamble. Expected {}, got {}.", FileT::version, preamble.version);
}


// Headers are always assumed to be at the very beginning of a mapping.
template<typename HeaderT>
void write_header_to(MappedRegion& mapping, const HeaderT& src) noexcept {
    std::memcpy(mapping_bytes(mapping), &src, sizeof(HeaderT));
    mapping.flush(0, sizeof(HeaderT));
}


[[nodiscard]]
constexpr auto next_aligned(uintptr_t value, size_t alignment)
    -> uintptr_t
{
    const auto div_ceil = (value / alignment) + bool(value % alignment);
    return div_ceil * alignment;
}


} // namespace




auto SkeletonFile::header() const noexcept
    -> Header&
{
    return *ptr_at_offset<Header>(mregion_, 0);
}


auto SkeletonFile::joints() const noexcept
    -> Span<Joint>
{
    const size_t offset = sizeof(Header);
    return { ptr_at_offset<Joint>(mregion_, offset), header().num_joints };
}


auto SkeletonFile::joint_names() const noexcept
    -> Span<ResourceName>
{
    const size_t offset =
        sizeof(Header) +
        sizeof(Joint) * header().num_joints;
    return { ptr_at_offset<ResourceName>(mregion_, offset), header().num_joints };
}


auto SkeletonFile::required_size(const Args& args) noexcept
    -> size_t
{
    const size_t num_joints  = args.num_joints;
    const size_t size_header = sizeof(Header);
    const size_t size_joints = sizeof(Joint) * num_joints;
    const size_t size_names  = sizeof(ResourceName) * num_joints;
    const size_t total_size  = size_header + size_joints + size_names;
    return total_size;
}


auto SkeletonFile::create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
    -> SkeletonFile
{
    assert(required_size(args) == mapped_region.get_size());
    const auto num_joints = args.num_joints;
    assert(num_joints <= Skeleton::max_joints);
    SkeletonFile file{ MOVE(mapped_region) };

    const Header header{
        .preamble   = ResourcePreamble::create(file_type, version, resource_type, self_uuid),
        ._reserved0 = {},
        .num_joints = num_joints,
        ._reserved1 = {},
    };

    write_header_to(file.mregion_, header);

    return file;
}


auto SkeletonFile::open(MappedRegion mapped_region)
    -> SkeletonFile
{
    SkeletonFile file{ MOVE(mapped_region) };
    const size_t file_size     = file.size_bytes();
    const size_t expected_size = required_size({ .num_joints = file.header().num_joints });
    throw_if_too_small_for_header<Header>(file_size);
    throw_if_mismatched_preamble<SkeletonFile>(file.header().preamble);
    throw_on_unexpected_size(expected_size, file_size);
    return file;
}








auto AnimationFile::header() const noexcept
    -> Header&
{
    return *ptr_at_offset<Header>(mregion_, 0);
}


auto AnimationFile::joint_span(size_t joint_id) const noexcept
    -> JointSpan&
{
    assert(joint_id < header().num_joints);
    const size_t offset =
        sizeof(Header) +
        sizeof(JointSpan) * joint_id;
    return *ptr_at_offset<JointSpan>(mregion_, offset);
}


auto AnimationFile::keyframes_header(size_t joint_id) const noexcept
    -> KeyframesHeader&
{
    assert(joint_id < header().num_joints);
    return *ptr_at_offset<KeyframesHeader>(mregion_, joint_span(joint_id).offset_bytes);
}


// Total size of KeyframesHeader + all Keys for a joint with `spec`.
static auto keyframes_size(const AnimationFile::KeySpec& spec) noexcept
    -> size_t
{
    return
        sizeof(AnimationFile::KeyframesHeader) +
        spec.num_pos_keys * sizeof(AnimationFile::KeyVec3) +
        spec.num_rot_keys * sizeof(AnimationFile::KeyQuat) +
        spec.num_sca_keys * sizeof(AnimationFile::KeyVec3);
}


auto AnimationFile::pos_keys(size_t joint_id) const noexcept
    -> Span<KeyVec3>
{
    assert(joint_id < header().num_joints);
    const auto& kfs = keyframes_header(joint_id);
    const size_t offset =
        joint_span(joint_id).offset_bytes +
        sizeof(KeyframesHeader);
    return { ptr_at_offset<KeyVec3>(mregion_, offset), kfs.num_pos_keys };
}


auto AnimationFile::rot_keys(size_t joint_id) const noexcept
    -> Span<KeyQuat>
{
    assert(joint_id < header().num_joints);
    const auto& kfs = keyframes_header(joint_id);
    const size_t offset =
        joint_span(joint_id).offset_bytes +
        sizeof(KeyframesHeader) +
        sizeof(KeyVec3) * kfs.num_pos_keys;
    return { ptr_at_offset<KeyQuat>(mregion_, offset), kfs.num_rot_keys };
}


auto AnimationFile::sca_keys(size_t joint_id) const noexcept
    -> Span<KeyVec3>
{
    assert(joint_id < header().num_joints);
    const auto& kfs = keyframes_header(joint_id);
    const size_t offset =
        joint_span(joint_id).offset_bytes +
        sizeof(KeyframesHeader) +
        sizeof(KeyVec3) * kfs.num_pos_keys +
        sizeof(KeyQuat) * kfs.num_rot_keys;
    return { ptr_at_offset<KeyVec3>(mregion_, offset), kfs.num_sca_keys };
}


auto AnimationFile::required_size(const Args& args) noexcept
    -> size_t
{
    const uint16_t num_joints       = args.key_specs.size();
    const size_t   header_size      = sizeof(Header);
    const size_t   joint_spans_size = num_joints * sizeof(JointSpan);

    size_t all_keyframes_size = 0;
    for (const KeySpec& spec : args.key_specs) {
        all_keyframes_size += keyframes_size(spec);
    }

    return header_size + joint_spans_size + all_keyframes_size;
}


auto AnimationFile::create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
    -> AnimationFile
{
    assert(required_size(args) == mapped_region.get_size());
    AnimationFile file{ MOVE(mapped_region) };

    const uint16_t num_joints = args.key_specs.size();

    // Write the header first, so that we could use num_joints().
    const Header header{
        .preamble      = ResourcePreamble::create(file_type, version, resource_type, self_uuid),
        .skeleton_uuid = {},
        .duration_s    = {},
        ._reserved1    = {},
        .num_joints    = num_joints,
    };

    write_header_to(file.mregion_, header);

    // Joint spans and Keyframes are still undefined, we will go one-by-one.

    size_t current_offset = sizeof(Header) + num_joints * sizeof(JointSpan);

    for (size_t joint_id{ 0 }; joint_id < num_joints; ++joint_id) {
        const KeySpec& spec = args.key_specs[joint_id];

        // Populate joint span first, else we won't be able to find the keyframes_ptr() correctly.
        JointSpan& span = file.joint_span(joint_id);
        span = {
            .offset_bytes = uint32_t(current_offset),
            .size_bytes   = uint32_t(keyframes_size(spec)),
        };

        // Now get keyframes.
        KeyframesHeader& kfs = file.keyframes_header(joint_id);
        kfs = {
            ._reserved0 = {},
            .num_pos_keys = spec.num_pos_keys,
            .num_rot_keys = spec.num_rot_keys,
            .num_sca_keys = spec.num_sca_keys,
        };

        current_offset += span.size_bytes;
    }

    return file;
}


auto AnimationFile::open(MappedRegion mapped_region)
    -> AnimationFile
{
    AnimationFile file{ MOVE(mapped_region) };
    const size_t file_size = file.size_bytes();

    throw_if_too_small_for_header<Header>(file_size);
    throw_if_mismatched_preamble<AnimationFile>(file.header().preamble);

    // Check if at least the spans are contained fully.
    const size_t num_joints = file.header().num_joints;
    const size_t with_spans_size = sizeof(Header) + num_joints * sizeof(JointSpan);
    if (file_size < with_spans_size) {
        throw InvalidResourceFile("Animation file too small to fit spans.");
    }

    // Check that the last joint info is contained. We only need the spans for this.
    // This is assuming that each span is placed in incrementing order...
    JointSpan& last_span = file.joint_span(num_joints - 1);
    const size_t expected_size = last_span.offset_bytes + last_span.size_bytes;
    throw_on_unexpected_size(expected_size, file_size);

    // TODO: We don't check each individual Keyframes structure for now.

    return file;
}








auto StaticMeshFile::header() const noexcept
    -> Header&
{
    return *ptr_at_offset<Header>(mregion_, 0);
}


auto StaticMeshFile::lod_span(size_t lod_id) const noexcept
    -> LODSpan&
{
    assert(lod_id < header().num_lods);
    return header().lods[lod_id];
}


auto StaticMeshFile::lod_verts_bytes(size_t lod_id) const noexcept
    -> Span<ubyte>
{
    assert(lod_id < header().num_lods);
    const LODSpan& span = lod_span(lod_id);
    const size_t offset = span.verts_offset_bytes;
    return { ptr_at_offset<ubyte>(mregion_, offset), span.verts_size_bytes };
}


auto StaticMeshFile::lod_elems_bytes(size_t lod_id) const noexcept
    -> Span<ubyte>
{
    assert(lod_id < header().num_lods);
    const LODSpan& span = lod_span(lod_id);
    const size_t offset = span.elems_offset_bytes;
    return { ptr_at_offset<ubyte>(mregion_, offset), span.elems_size_bytes };
}


auto StaticMeshFile::required_size(const Args& args) noexcept
    -> size_t
{
    size_t total_size = sizeof(Header);
    for (const LODSpec& spec : args.lod_specs) {
        total_size = next_aligned(total_size, alignof(vertex_type));
        total_size += spec.verts_size_bytes;
        total_size = next_aligned(total_size, alignof(element_type));
        total_size += spec.elems_size_bytes;
    }

    return total_size;
}


auto StaticMeshFile::create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
    -> StaticMeshFile
{
    assert(required_size(args) == mapped_region.get_size());

    const size_t num_lods = args.lod_specs.size();
    assert(num_lods <= max_lods);
    assert(num_lods > 0);

    StaticMeshFile file{ MOVE(mapped_region) };

    Header header{
        .preamble   = ResourcePreamble::create(file_type, version, resource_type, self_uuid),
        ._reserved0 = {},
        .num_lods   = uint8_t(num_lods),
        ._reserved1 = {},
        .aabb       = {},
        ._reserved2 = {},
        .lods       = {}, // NOTE: Zero-init here. Fill later.
    };

    // Populate spans. From lowres LODs to hires.
    // NOTE: Uh, sorry for the "goes-to operator", it actually works here...
    size_t current_offset = sizeof(Header);
    for (size_t lod_id{ num_lods }; lod_id --> 0;) {
        LODSpan&       span = header.lods   [lod_id];
        const LODSpec& spec = args.lod_specs[lod_id];

        span.num_verts = spec.num_verts;
        span.num_elems = spec.num_elems;

        current_offset          = next_aligned(current_offset, alignof(vertex_type));
        span.verts_offset_bytes = current_offset;
        span.verts_size_bytes   = spec.verts_size_bytes;
        current_offset         += spec.verts_size_bytes;

        current_offset          = next_aligned(current_offset, alignof(element_type));
        span.elems_offset_bytes = current_offset;
        span.elems_size_bytes   = spec.elems_size_bytes;
        current_offset         += spec.elems_size_bytes;
    }

    write_header_to(file.mregion_, header);

    return file;
}


auto StaticMeshFile::open(MappedRegion mapped_region)
    -> StaticMeshFile
{
    StaticMeshFile file{ MOVE(mapped_region) };
    const size_t file_size = file.size_bytes();
    throw_if_too_small_for_header<Header>(file_size);
    throw_if_mismatched_preamble<StaticMeshFile>(file.header().preamble);

    const Header& header = file.header();

    // Check lod limit.
    if (header.num_lods > max_lods or header.num_lods == 0)
        throw InvalidResourceFile("Mesh file specifies invalid number of LODs.");

    // Check size.
    // Also check that each vertex bytesize is multiple of sizeof(VertexT).
    size_t expected_size = sizeof(Header);
    for (size_t lod_id{ 0 }; lod_id < header.num_lods; ++lod_id) {
        const size_t verts_bytes = header.lods[lod_id].verts_size_bytes;
        const size_t elems_bytes = header.lods[lod_id].elems_size_bytes;
        if (verts_bytes % sizeof(vertex_type))
            throw InvalidResourceFile("Mesh file contains invalid vertex data.");
        expected_size = next_aligned(expected_size, alignof(vertex_type))  + verts_bytes;
        expected_size = next_aligned(expected_size, alignof(element_type)) + elems_bytes;
    }

    throw_on_unexpected_size(expected_size, file_size);

    return file;
}







auto SkinnedMeshFile::header() const noexcept
    -> Header&
{
    return *ptr_at_offset<Header>(mregion_, 0);
}


auto SkinnedMeshFile::lod_span(size_t lod_id) const noexcept
    -> LODSpan&
{
    assert(lod_id < header().num_lods);
    return header().lods[lod_id];
}


auto SkinnedMeshFile::lod_verts_bytes(size_t lod_id) const noexcept
    -> Span<ubyte>
{
    assert(lod_id < header().num_lods);
    const LODSpan& span = lod_span(lod_id);
    const size_t offset = span.verts_offset_bytes;
    return { ptr_at_offset<ubyte>(mregion_, offset), span.verts_size_bytes };
}


auto SkinnedMeshFile::lod_elems_bytes(size_t lod_id) const noexcept
    -> Span<ubyte>
{
    assert(lod_id < header().num_lods);
    const LODSpan& span = lod_span(lod_id);
    const size_t offset = span.elems_offset_bytes;
    return { ptr_at_offset<ubyte>(mregion_, offset), span.elems_size_bytes };
}


auto SkinnedMeshFile::required_size(const Args& args) noexcept
    -> size_t
{
    size_t total_size = sizeof(Header);
    for (const LODSpec& spec : args.lod_specs) {
        total_size = next_aligned(total_size, alignof(vertex_type));
        total_size += spec.verts_size_bytes;
        total_size = next_aligned(total_size, alignof(element_type));
        total_size += spec.elems_size_bytes;
    }

    return total_size;
}


auto SkinnedMeshFile::create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
    -> SkinnedMeshFile
{
    assert(required_size(args) == mapped_region.get_size());

    const size_t num_lods = args.lod_specs.size();
    assert(num_lods <= max_lods);
    assert(num_lods > 0);

    SkinnedMeshFile file{ MOVE(mapped_region) };

    Header header{
        .preamble      = ResourcePreamble::create(file_type, version, resource_type, self_uuid),
        .skeleton_uuid = args.skeleton_uuid,
        ._reserved0    = {},
        .num_lods      = uint8_t(num_lods),
        ._reserved1    = {},
        .aabb          = {},
        ._reserved2    = {},
        .lods          = {}, // NOTE: Zero-init here. Fill later.
    };

    // Populate spans. From lowres LODs to hires.
    // NOTE: Uh, sorry for the "goes-to operator", it actually works here...
    size_t current_offset = sizeof(Header);
    for (size_t lod_id{ num_lods }; lod_id --> 0;) {
        LODSpan&       span = header.lods   [lod_id];
        const LODSpec& spec = args.lod_specs[lod_id];

        span.num_verts = spec.num_verts;
        span.num_elems = spec.num_elems;

        current_offset          = next_aligned(current_offset, alignof(vertex_type));
        span.verts_offset_bytes = current_offset;
        span.verts_size_bytes   = spec.verts_size_bytes;
        current_offset         += spec.verts_size_bytes;

        current_offset          = next_aligned(current_offset, alignof(element_type));
        span.elems_offset_bytes = current_offset;
        span.elems_size_bytes   = spec.elems_size_bytes;
        current_offset         += spec.elems_size_bytes;
    }

    write_header_to(file.mregion_, header);

    return file;
}


auto SkinnedMeshFile::open(MappedRegion mapped_region)
    -> SkinnedMeshFile
{
    SkinnedMeshFile file{ MOVE(mapped_region) };
    const size_t file_size = file.size_bytes();
    throw_if_too_small_for_header<Header>(file_size);
    throw_if_mismatched_preamble<SkinnedMeshFile>(file.header().preamble);

    const Header& header = file.header();

    if (header.num_lods > max_lods or header.num_lods == 0)
        throw InvalidResourceFile("Mesh file specifies invalid number of LODs.");

    size_t expected_size = sizeof(Header);
    for (size_t lod_id{ 0 }; lod_id < header.num_lods; ++lod_id) {
        const size_t verts_bytes = header.lods[lod_id].verts_size_bytes;
        const size_t elems_bytes = header.lods[lod_id].elems_size_bytes;
        if (verts_bytes % sizeof(vertex_type))
            throw InvalidResourceFile("Mesh file contains invalid vertex data.");
        expected_size = next_aligned(expected_size, alignof(vertex_type))  + verts_bytes;
        expected_size = next_aligned(expected_size, alignof(element_type)) + elems_bytes;
    }

    throw_on_unexpected_size(expected_size, file_size);

    return file;
}








auto TextureFile::header() const noexcept
    -> Header&
{
    return *ptr_at_offset<Header>(mregion_, 0);
}


auto TextureFile::mip_span(size_t mip_id) const noexcept
    -> MIPSpan&
{
    assert(mip_id < header().num_mips);
    return header().mips[mip_id];
}


auto TextureFile::mip_bytes(size_t mip_id) const noexcept
    -> Span<ubyte>
{
    assert(mip_id < header().num_mips);
    const MIPSpan& span = mip_span(mip_id);
    const size_t offset = span.offset_bytes;
    return { ptr_at_offset<ubyte>(mregion_, offset), span.size_bytes };
}


auto TextureFile::required_size(const Args& args) noexcept
    -> size_t
{
    size_t total_size = sizeof(Header);
    for (const MIPSpec& spec : args.mip_specs) {
        total_size += spec.size_bytes;
    }
    return total_size;
}


auto TextureFile::create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
    -> TextureFile
{
    assert(required_size(args) == mapped_region.get_size());

    const size_t num_mips = args.mip_specs.size();
    assert(num_mips <= max_mips);
    assert(num_mips > 0);
    assert(args.num_channels <= 4);

    TextureFile file{ MOVE(mapped_region) };

    Header header{
        .preamble     = ResourcePreamble::create(file_type, version, resource_type, self_uuid),
        .num_channels = args.num_channels,
        .colorspace   = args.colorspace,
        ._reserved0   = {},
        .num_mips     = uint8_t(num_mips),
        ._reserved1   = {},
        .mips         = {}, // NOTE: Zero-init here. Fill later.
    };

    // Populate spans. From lowres MIPs to hires.
    size_t current_offset{ sizeof(Header) };
    for (size_t mip_id{ num_mips }; mip_id --> 0;) {
        MIPSpan&       span = header.mips   [mip_id];
        const MIPSpec& spec = args.mip_specs[mip_id];

        span.offset_bytes  = current_offset;
        span.size_bytes    = spec.size_bytes;
        span.width  = spec.width_pixels;
        span.height = spec.height_pixels;
        span.encoding      = spec.encoding;

        current_offset += span.size_bytes;
    }

    write_header_to(file.mregion_, header);

    return file;
}


auto TextureFile::open(MappedRegion mapped_region)
    -> TextureFile
{
    TextureFile file{ MOVE(mapped_region) };

    const size_t file_size = file.size_bytes();
    throw_if_too_small_for_header<Header>(file_size);
    throw_if_mismatched_preamble<TextureFile>(file.header().preamble);

    const auto& header = file.header();

    // Check mip limit.
    if (header.num_mips > max_mips or header.num_mips == 0)
        throw InvalidResourceFile("Texture file specifies invalid number of MIPs.");


    // Check channel limit.
    if (header.num_channels > 4 or header.num_channels == 0)
        throw InvalidResourceFile("Texture file specifies invalid number of channels.");


    // Check storage formats.
    for (const size_t mip_id : irange(header.num_mips)) {
        if (to_underlying(file.mip_span(mip_id).encoding) >= enum_size<Encoding>())
            throw InvalidResourceFile("Texture file has invalid encoding.");
    }

    // Check size.
    size_t expected_size = sizeof(Header);
    for (size_t mip_id{ 0 }; mip_id < header.num_mips; ++mip_id) {
        const size_t size_bytes = header.mips[mip_id].size_bytes;
        expected_size += size_bytes;
    }

    throw_on_unexpected_size(expected_size, file_size);

    return file;
}


} // namespace josh
