#include "DefaultResourceFiles.hpp"
#include "Common.hpp"
#include "ContainerUtils.hpp"
#include "FileMapping.hpp"
#include "EnumUtils.hpp"
#include "CategoryCasts.hpp"
#include "Ranges.hpp"
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
        throw error::InvalidResourceFile("Resource file is too small to contain header information.");
    }
}


void throw_on_unexpected_size(size_t expected, size_t real) noexcept(false) {
    if (real != expected) {
        throw error::InvalidResourceFile(fmt::format("Resource file unexpected size. Expected {}, got {}.", expected, real));
    }
}


// Headers are always assumed to be at the very beginning of a mapping.
template<typename HeaderT>
void write_header_to(MappedRegion& mapping, const HeaderT& src) noexcept {
    std::memcpy(mapping_bytes(mapping), &src, sizeof(HeaderT));
    mapping.flush(0, sizeof(HeaderT));
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

    // Check if at least preamble is contained fully.
    const size_t num_joints = file.header().num_joints;
    const size_t preamble_size = sizeof(Header) + num_joints * sizeof(JointSpan);
    if (file_size < preamble_size) {
        throw error::InvalidResourceFile("Animation file too small.");
    }

    // Check that the last joint info is contained. We only need the preamble for this.
    // This is assuming that each span is placed in incrementing order...
    JointSpan& last_span = file.joint_span(num_joints - 1);
    const size_t expected_size = last_span.offset_bytes + last_span.size_bytes;
    throw_on_unexpected_size(expected_size, file_size);

    // We don't check each individual Keyframes structure for now.

    return file;
}








auto MeshFile::header() const noexcept
    -> Header&
{
    return *ptr_at_offset<Header>(mregion_, 0);
}


auto MeshFile::lod_span(size_t lod_id) const noexcept
    -> LODSpan&
{
    assert(lod_id < header().num_lods);
    return header().lods[lod_id];
}


auto MeshFile::lod_verts_bytes(size_t lod_id) const noexcept
    -> Span<ubyte>
{
    assert(lod_id < header().num_lods);
    const LODSpan& span = lod_span(lod_id);
    const size_t offset = span.offset_bytes;
    return { ptr_at_offset<ubyte>(mregion_, offset), span.verts_bytes };
}


auto MeshFile::lod_elems_bytes(size_t lod_id) const noexcept
    -> Span<ubyte>
{
    assert(lod_id < header().num_lods);
    const LODSpan& span = lod_span(lod_id);
    const size_t offset = span.offset_bytes + span.verts_bytes;
    return { ptr_at_offset<ubyte>(mregion_, offset), span.elems_bytes };
}


static auto vert_size(MeshFile::VertexLayout layout)
    -> size_t
{
    switch (layout) {
        using enum MeshFile::VertexLayout;
        case Static:  return sizeof(MeshFile::layout_traits<Static>::type);
        case Skinned: return sizeof(MeshFile::layout_traits<Skinned>::type);
        default: safe_unreachable(); // NOTE: Validate before calling, buddy.
    }
}


auto MeshFile::required_size(const Args& args) noexcept
    -> size_t
{
    size_t total_size = sizeof(Header);
    for (const LODSpec& spec : args.lod_specs) {
        total_size += spec.verts_bytes;
        total_size += spec.elems_bytes;
    }

    return total_size;
}


auto MeshFile::create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
    -> MeshFile
{
    assert(required_size(args) == mapped_region.get_size());

    const size_t num_lods = args.lod_specs.size();
    assert(num_lods <= max_lods);
    assert(num_lods > 0);

    MeshFile file{ MOVE(mapped_region) };

    Header header{
        .preamble   = ResourcePreamble::create(file_type, version, resource_type, self_uuid),
        .skeleton_uuid   = {},
        .layout     = args.layout,
        .num_lods   = uint8_t(num_lods),
        ._reserved0 = {},
        .aabb       = {},
        .lods       = {}, // NOTE: Zero-init here. Fill later.
    };

    // FIXME: We need to guarantee that uncompressed vertex data
    // is properly ALIGNED in the file! It should be possible to type pun.
    // If some LODs are compressed and some aren't, this might not be the case.

    // Populate spans. From lowres LODs to hires.
    // NOTE: Uh, sorry for the "goes-to operator", it actually works here...
    size_t current_offset = sizeof(Header);
    for (size_t lod_id{ num_lods }; lod_id --> 0;) {
        LODSpan&       span = header.lods   [lod_id];
        const LODSpec& spec = args.lod_specs[lod_id];

        span.offset_bytes = current_offset;
        span.num_verts    = spec.num_verts;
        span.num_elems    = spec.num_elems;
        span.verts_bytes  = spec.verts_bytes;
        span.elems_bytes  = spec.elems_bytes;
        span.compression  = spec.compression;

        current_offset += span.verts_bytes + span.elems_bytes;
    }

    write_header_to(file.mregion_, header);

    return file;
}


auto MeshFile::open(MappedRegion mapped_region)
    -> MeshFile
{
    MeshFile file{ MOVE(mapped_region) };
    const size_t file_size = file.size_bytes();
    throw_if_too_small_for_header<Header>(file_size);

    const Header& header = file.header();

    // Check layout type.
    const bool valid_layout =
        to_underlying(file.header().layout) < enum_size<VertexLayout>();

    if (!valid_layout) {
        throw InvalidResourceFile("Mesh file has invalid layout.");
    }

    // Check lod limit.
    if (header.num_lods > max_lods || header.num_lods == 0) {
        throw InvalidResourceFile("Mesh file specifies invalid number of LODs.");
    }

    // Check size.
    // Also check that each vertex bytesize is multiple of sizeof(VertexT).
    size_t expected_size = sizeof(Header);
    for (size_t lod_id{ 0 }; lod_id < header.num_lods; ++lod_id) {
        const size_t verts_bytes = header.lods[lod_id].verts_bytes;
        const size_t elems_bytes = header.lods[lod_id].elems_bytes;
        if (verts_bytes % vert_size(header.layout)) {
            throw InvalidResourceFile("Mesh file contains invalid vertex data.");
        }
        expected_size += verts_bytes;
        expected_size += elems_bytes;
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

    Header& header = file.header();

    // Check mip limit.
    if (header.num_mips > max_mips || header.num_mips == 0) {
        throw error::InvalidResourceFile("Texture file specifies invalid number of MIPs.");
    }

    // Check channel limit.
    if (header.num_channels > 4 || header.num_channels == 0) {
        throw error::InvalidResourceFile("Texture file specifies invalid number of channels.");
    }

    // Check storage formats.
    for (const size_t mip_id : irange(header.num_mips)) {
        if (to_underlying(file.mip_span(mip_id).encoding) >= enum_size<Encoding>()) {
            throw error::InvalidResourceFile("Texture file has invalid encoding.");
        }
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
