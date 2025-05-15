#include "ResourceFiles.hpp"
#include "Common.hpp"
#include "FileMapping.hpp"
#include "AABB.hpp"
#include "EnumUtils.hpp"
#include "Filesystem.hpp"
#include "CategoryCasts.hpp"
#include "Ranges.hpp"
#include "ReadFile.hpp"
#include "UUID.hpp"
#include <fmt/core.h>
#include <fmt/std.h>
#include <algorithm>
#include <bit>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <streambuf>
#include <ios>
#include <new>


namespace josh {


namespace bip = boost::interprocess;


namespace {


[[nodiscard]]
auto create_file(const Path& path, size_t size_bytes)
    -> std::filebuf
{
    std::filebuf file{};
    const std::ios::openmode mode = // "w+b"
        std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out;
    if (file.open(path, mode)) {
        file.pubseekoff(ptrdiff_t(size_bytes - 1), std::ios::beg);
        file.sputc(0);
        return file;
    } else {
        // TODO: Totally wrong error type.
        throw error::FileReadingError(path);
    }

}


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


template<typename HeaderT>
[[nodiscard]] auto open_file_mapping(const Path& path)
    -> MappedRegion
{
    // NOTE: Mapped regions persist even after the file_mapping was destroyed,
    // so we drop the file_mapping at the end of this scope.

    FileMapping  file{ path.c_str(), bip::read_write };
    MappedRegion mapping{ file, bip::read_write };
    mapping.advise(MappedRegion::advice_sequential); // TODO: Useful?

    const size_t file_size = mapping.get_size();

    throw_if_too_small_for_header<HeaderT>(file_size, path);

    return mapping;
}


[[nodiscard]] auto create_file_mapping(const Path& path, size_t file_size)
    -> MappedRegion
{
    std::filebuf file = create_file(path, file_size);

    // NOTE: This actually writes to disk. Since the file_mapping opens by-path
    // again, we need to flush the "resize" before that, else we are going to
    // open an "empty" file. Mapping it would fail.
    file.pubsync();

    FileMapping  file_mapping{ path.c_str(), bip::read_write };
    MappedRegion mapping{ file_mapping, bip::read_write };
    mapping.advise(MappedRegion::advice_sequential);

    return mapping;
}


// Headers are always assumed to be at the very beginning of a mapping.
template<typename HeaderT>
void write_header_to(MappedRegion& mapping, const HeaderT& src) noexcept {
    std::memcpy(mapping_bytes(mapping), &src, sizeof(HeaderT));
    mapping.flush(0, sizeof(HeaderT));
}


} // namespace


auto ResourcePreamble::create(
    ResourceType resource_type,
    const UUID&  self_uuid) noexcept
        -> ResourcePreamble
{
    const char magic[4] = { 'j', 'o', 's', 'h' };
    return {
        ._magic        = std::bit_cast<uint32_t>(magic),
        .resource_type = resource_type,
        .self_uuid     = self_uuid,
    };
}


auto ResourceName::from_view(StrView sv) noexcept
    -> ResourceName
{
    const size_t length = std::min(max_length, sv.length());
    ResourceName result{
        .length = uint8_t(length),
        .name   = {},
    };
    std::ranges::copy(sv.substr(0, length), result.name);
    return result;
}


auto ResourceName::from_cstr(const char* cstr) noexcept
    -> ResourceName
{
    size_t      space  = max_length;
    const char* cur    = cstr;
    while (space && *cur != '\0') {
        --space;
        ++cur;
    }
    const size_t length = cur - cstr;
    ResourceName result{
        .length = uint8_t(length),
        .name   = {},
    };
    std::ranges::copy(std::ranges::subrange(cstr, cur), result.name);
    return result;
}


auto ResourceName::view() const noexcept
    -> StrView
{
    return { name, length };
}






SkeletonFile::SkeletonFile(MappedRegion mapping)
    : mapping_{ MOVE(mapping) }
{}


auto SkeletonFile::header_ptr() const noexcept
    -> Header*
{
    return please_type_pun<Header>(mapping_.get_address());
}


auto SkeletonFile::joints_ptr() const noexcept
    -> Joint*
{
    const size_t offset = sizeof(Header);
    return please_type_pun<Joint>(mapping_bytes(mapping_) + offset);
}


auto SkeletonFile::joint_names_ptr() const noexcept
    -> ResourceName*
{
    const size_t offset =
        sizeof(Header) +
        sizeof(Joint) * num_joints();
    return please_type_pun<ResourceName>(mapping_bytes(mapping_) + offset);
}


auto SkeletonFile::num_joints() const noexcept
    -> uint16_t
{
    return header_ptr()->num_joints;
}


auto SkeletonFile::joints() noexcept
    -> Span<Joint>
{
    return { joints_ptr(), num_joints() };
}


auto SkeletonFile::joints() const noexcept
    -> Span<const Joint>
{
    return { joints_ptr(), num_joints() };
}


auto SkeletonFile::joint_names() noexcept
    -> Span<ResourceName>
{
    return { joint_names_ptr(), num_joints() };
}


auto SkeletonFile::joint_names() const noexcept
    -> Span<const ResourceName>
{
    return { joint_names_ptr(), num_joints() };
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
        .preamble   = ResourcePreamble::create(resource_type, self_uuid),
        ._reserved0 = {},
        .num_joints = num_joints,
        ._padding0  = {},
    };

    write_header_to(file.mapping_, header);

    return file;
}


auto SkeletonFile::open(MappedRegion mapped_region)
    -> SkeletonFile
{
    SkeletonFile file{ MOVE(mapped_region) };
    const size_t file_size     = file.size_bytes();
    const size_t expected_size = required_size({ .num_joints = file.num_joints() });
    throw_if_too_small_for_header<Header>(file_size);
    throw_on_unexpected_size(expected_size, file_size);
    return file;
}








AnimationFile::AnimationFile(MappedRegion mapping)
    : mapping_{ MOVE(mapping) }
{}


auto AnimationFile::header_ptr() const noexcept
    -> Header*
{
    return please_type_pun<Header>(mapping_.get_address());
}


auto AnimationFile::joint_span_ptr(size_t joint_id) const noexcept
    -> JointSpan*
{
    assert(joint_id < num_joints());
    const size_t initial_offset = sizeof(Header);
    JointSpan* first = please_type_pun<JointSpan>(initial_offset + mapping_bytes(mapping_));
    return first + joint_id;
}


auto AnimationFile::keyframes_size(const KeySpec& spec) noexcept
    -> size_t
{
    return
        sizeof(KeyframesHeader) +
        spec.num_pos_keys * sizeof(KeyVec3) +
        spec.num_rot_keys * sizeof(KeyQuat) +
        spec.num_sca_keys * sizeof(KeyVec3);
}


auto AnimationFile::keyframes_ptr(size_t joint_id) const noexcept
    -> KeyframesHeader*
{
    const uint32_t offset_bytes = joint_span_ptr(joint_id)->offset_bytes;
    return please_type_pun<KeyframesHeader>(mapping_bytes(mapping_) + offset_bytes);
}


auto AnimationFile::pos_keys_ptr(size_t joint_id) const noexcept
    -> KeyVec3*
{
    auto* kfs = keyframes_ptr(joint_id);
    const size_t offset_bytes = sizeof(KeyframesHeader);
    return please_type_pun<KeyVec3>((ubyte*)kfs + offset_bytes);
}


auto AnimationFile::rot_keys_ptr(size_t joint_id) const noexcept
    -> KeyQuat*
{
    auto* kfs = keyframes_ptr(joint_id);
    const size_t offset_bytes =
        sizeof(KeyframesHeader) +
        sizeof(KeyVec3) * kfs->num_pos_keys;
    return please_type_pun<KeyQuat>((byte*)kfs + offset_bytes);
}


auto AnimationFile::sca_keys_ptr(size_t joint_id) const noexcept
    -> KeyVec3*
{
    auto* kfs = keyframes_ptr(joint_id);
    const size_t offset_bytes =
        sizeof(KeyframesHeader) +
        sizeof(KeyVec3) * kfs->num_pos_keys +
        sizeof(KeyQuat) * kfs->num_rot_keys;
    return please_type_pun<KeyVec3>((byte*)kfs + offset_bytes);
}


auto AnimationFile::skeleton_uuid() noexcept
    -> UUID&
{
    return header_ptr()->skeleton_uuid;
}


auto AnimationFile::skeleton_uuid() const noexcept
    -> const UUID&
{
    return header_ptr()->skeleton_uuid;
}


auto AnimationFile::duration_s() noexcept
    -> float&
{
    return header_ptr()->duration_s;
}


auto AnimationFile::duration_s() const noexcept
    -> const float&
{
    return header_ptr()->duration_s;
}


auto AnimationFile::num_joints() const noexcept
    -> uint16_t
{
    return header_ptr()->num_joints;
}


auto AnimationFile::num_pos_keys(size_t joint_id) const noexcept
    -> uint32_t
{
    return keyframes_ptr(joint_id)->num_pos_keys;
}


auto AnimationFile::num_rot_keys(size_t joint_id) const noexcept
    -> uint32_t
{
    return keyframes_ptr(joint_id)->num_rot_keys;
}


auto AnimationFile::num_sca_keys(size_t joint_id) const noexcept
    -> uint32_t
{
    return keyframes_ptr(joint_id)->num_sca_keys;
}


auto AnimationFile::pos_keys(size_t joint_id) noexcept
    -> Span<KeyVec3>
{
    return { pos_keys_ptr(joint_id), num_pos_keys(joint_id) };
}


auto AnimationFile::pos_keys(size_t joint_id) const noexcept
    -> Span<const KeyVec3>
{
    return { pos_keys_ptr(joint_id), num_pos_keys(joint_id) };
}


auto AnimationFile::rot_keys(size_t joint_id) noexcept
    -> Span<KeyQuat>
{
    return { rot_keys_ptr(joint_id), num_rot_keys(joint_id) };
}


auto AnimationFile::rot_keys(size_t joint_id) const noexcept
    -> Span<const KeyQuat>
{
    return { rot_keys_ptr(joint_id), num_rot_keys(joint_id) };
}


auto AnimationFile::sca_keys(size_t joint_id) noexcept
    -> Span<KeyVec3>
{
    return { sca_keys_ptr(joint_id), num_sca_keys(joint_id) };
}


auto AnimationFile::sca_keys(size_t joint_id) const noexcept
    -> Span<const KeyVec3>
{
    return { sca_keys_ptr(joint_id), num_sca_keys(joint_id) };
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
        .preamble      = ResourcePreamble::create(resource_type, self_uuid),
        .skeleton_uuid = {},
        .duration_s    = {},
        ._reserved1    = {},
        .num_joints    = num_joints,
    };

    write_header_to(file.mapping_, header);

    // Joint spans and Keyframes are still undefined, we will go one-by-one.

    size_t current_offset = sizeof(Header) + num_joints * sizeof(JointSpan);

    for (size_t joint_id{ 0 }; joint_id < num_joints; ++joint_id) {
        const KeySpec& spec = args.key_specs[joint_id];

        // Populate joint span first, else we won't be able to find the keyframes_ptr() correctly.
        JointSpan& span = *file.joint_span_ptr(joint_id);
        span = {
            .offset_bytes = uint32_t(current_offset),
            .size_bytes   = uint32_t(keyframes_size(spec)),
        };

        // Now get keyframes.
        KeyframesHeader& kfs = *file.keyframes_ptr(joint_id);
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
    const size_t preamble_size = sizeof(Header) + file.num_joints() * sizeof(JointSpan);
    if (file_size < preamble_size) {
        throw error::InvalidResourceFile("Animation file too small.");
    }

    // Check that the last joint info is contained. We only need the preamble for this.
    // This is assuming that each span is placed in incrementing order...
    JointSpan& last_span = *file.joint_span_ptr(file.num_joints() - 1);
    const size_t expected_size = last_span.offset_bytes + last_span.size_bytes;
    throw_on_unexpected_size(expected_size, file_size);

    // We don't check each individual Keyframes structure for now.

    return file;
}










MeshFile::MeshFile(MappedRegion mregion)
    : mapping_{ MOVE(mregion) }
{}


auto MeshFile::header_ptr() const noexcept
    -> Header*
{
    return please_type_pun<Header>(mapping_.get_address());
}


using VertexLayout = MeshFile::VertexLayout;


auto MeshFile::lod_verts_ptr(size_t lod_id) const noexcept
    -> ubyte*
{
    assert(lod_id < num_lods());
    const LODSpan& span = header_ptr()->lods[lod_id];
    const size_t offset = span.offset_bytes;
    return mapping_bytes(mapping_) + offset;
}


auto MeshFile::lod_elems_ptr(size_t lod_id) const noexcept
    -> ubyte*
{
    assert(lod_id < num_lods());
    const LODSpan& span = header_ptr()->lods[lod_id];
    const size_t offset = span.offset_bytes + span.verts_bytes;
    return mapping_bytes(mapping_) + offset;
}


auto MeshFile::vert_size(VertexLayout layout) noexcept
    -> size_t
{
    switch (layout) {
        using enum VertexLayout;
        case Static:  return sizeof(layout_traits<Static>::type);
        case Skinned: return sizeof(layout_traits<Skinned>::type);
        default: std::terminate(); // NOTE: Validate before calling, buddy.
    }
}


auto MeshFile::skeleton_uuid() noexcept
    -> UUID&
{
    return header_ptr()->skeleton;
}


auto MeshFile::skeleton_uuid() const noexcept
    -> const UUID&
{
    return header_ptr()->skeleton;
}


auto MeshFile::aabb() noexcept
    -> LocalAABB&
{
    return header_ptr()->aabb;
}


auto MeshFile::aabb() const noexcept
    -> const LocalAABB&
{
    return header_ptr()->aabb;
}


auto MeshFile::layout() const noexcept
    -> VertexLayout
{
    return header_ptr()->layout;
}


auto MeshFile::num_lods() const noexcept
    -> uint16_t
{
    return header_ptr()->num_lods;
}


auto MeshFile::num_verts(size_t lod_id) const noexcept
    -> uint32_t
{
    assert(lod_id < num_lods());
    return header_ptr()->lods[lod_id].num_verts;
}


auto MeshFile::num_elems(size_t lod_id) const noexcept
    -> uint32_t
{
    assert(lod_id < num_lods());
    return header_ptr()->lods[lod_id].num_elems;
}


auto MeshFile::lod_spec(size_t lod_id) const noexcept
    -> LODSpec
{
    assert(lod_id < num_lods());
    const LODSpan& span = header_ptr()->lods[lod_id];
    return {
        .num_verts   = span.num_verts,
        .num_elems   = span.num_elems,
        .verts_bytes = span.verts_bytes,
        .elems_bytes = span.elems_bytes,
        .compression = span.compression,
        ._reserved0  = {},
    };
}


auto MeshFile::lod_compression(size_t lod_id) const noexcept
    -> Compression
{
    assert(lod_id < num_lods());
    return header_ptr()->lods[lod_id].compression;
}


auto MeshFile::lod_verts_size_bytes(size_t lod_id) const noexcept
    -> uint32_t
{
    assert(lod_id < num_lods());
    return header_ptr()->lods[lod_id].verts_bytes;
}


auto MeshFile::lod_elems_size_bytes(size_t lod_id) const noexcept
    -> uint32_t
{
    assert(lod_id < num_lods());
    return header_ptr()->lods[lod_id].elems_bytes;
}


auto MeshFile::lod_verts_bytes(size_t lod_id) noexcept
    -> Span<ubyte>
{
    return { lod_verts_ptr(lod_id), lod_verts_size_bytes(lod_id) };
}


auto MeshFile::lod_verts_bytes(size_t lod_id) const noexcept
    -> Span<const ubyte>
{
    return { lod_verts_ptr(lod_id), lod_verts_size_bytes(lod_id) };
}


auto MeshFile::lod_elems_bytes(size_t lod_id) noexcept
    -> Span<ubyte>
{
    return { lod_elems_ptr(lod_id), lod_elems_size_bytes(lod_id) };
}


auto MeshFile::lod_elems_bytes(size_t lod_id) const noexcept
    -> Span<const ubyte>
{
    return { lod_elems_ptr(lod_id), lod_elems_size_bytes(lod_id) };
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
        .preamble = ResourcePreamble::create(resource_type, self_uuid),
        .skeleton = {},
        .layout   = args.layout,
        .num_lods = uint16_t(num_lods),
        .aabb     = {},
        .lods     = {}, // NOTE: Zero-init here. Fill later.
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

    write_header_to(file.mapping_, header);

    return file;
}


auto MeshFile::open(MappedRegion mapped_region)
    -> MeshFile
{
    MeshFile file{ MOVE(mapped_region) };
    const size_t file_size = file.size_bytes();
    throw_if_too_small_for_header<Header>(file_size);

    const Header& header = *file.header_ptr();

    // Check layout type.
    const bool valid_layout =
        to_underlying(file.layout()) < to_underlying(VertexLayout::_count);

    if (!valid_layout) {
        throw error::InvalidResourceFile("Mesh file has invalid layout.");
    }

    // Check lod limit.
    if (header.num_lods > max_lods || header.num_lods == 0) {
        throw error::InvalidResourceFile("Mesh file specifies invalid number of LODs.");
    }

    // Check size.
    // Also check that each vertex bytesize is multiple of sizeof(VertexT).
    size_t expected_size = sizeof(Header);
    for (size_t lod_id{ 0 }; lod_id < header.num_lods; ++lod_id) {
        const size_t verts_bytes = header.lods[lod_id].verts_bytes;
        const size_t elems_bytes = header.lods[lod_id].elems_bytes;
        if (verts_bytes % vert_size(header.layout)) {
            throw error::InvalidResourceFile("Mesh file contains invalid vertex data.");
        }
        expected_size += verts_bytes;
        expected_size += elems_bytes;
    }

    throw_on_unexpected_size(expected_size, file_size);

    return file;
}












TextureFile::TextureFile(MappedRegion mregion)
    : mapping_{ MOVE(mregion) }
{}


auto TextureFile::header_ptr() const noexcept
    -> Header*
{
    return please_type_pun<Header>(mapping_.get_address());
}


using StorageFormat = TextureFile::StorageFormat;


auto TextureFile::mip_bytes_ptr(size_t mip_id) const noexcept
    -> ubyte*
{
    assert(mip_id < num_mips());
    const MIPSpan& span = header_ptr()->mips[mip_id];
    const size_t offset = span.offset_bytes;
    return mapping_bytes(mapping_) + offset;
}


auto TextureFile::format(size_t mip_id) const noexcept
    -> StorageFormat
{
    assert(mip_id < num_mips());
    return header_ptr()->mips[mip_id].format;
}


auto TextureFile::num_mips() const noexcept
    -> uint8_t
{
    return header_ptr()->num_mips;
}


auto TextureFile::num_channels() const noexcept
    -> uint8_t
{
    return header_ptr()->num_channels;
}


auto TextureFile::colorspace() const noexcept
    -> const Colorspace&
{
    return header_ptr()->colorspace;
}


auto TextureFile::colorspace() noexcept
    -> Colorspace&
{
    return header_ptr()->colorspace;
}


auto TextureFile::mip_spec(size_t mip_id) const noexcept
    -> MIPSpec
{
    assert(mip_id < num_mips());
    const MIPSpan& span = header_ptr()->mips[mip_id];
    return {
        .size_bytes    = span.size_bytes,
        .width_pixels  = span.width_pixels,
        .height_pixels = span.height_pixels,
        .format        = span.format,
    };
}


auto TextureFile::resolution(size_t mip_id) const noexcept
    -> Size2I
{
    assert(mip_id < num_mips());
    const MIPSpan& span = header_ptr()->mips[mip_id];
    return { span.width_pixels, span.height_pixels };
}


auto TextureFile::mip_size_bytes(size_t mip_id) const noexcept
    -> uint32_t
{
    assert(mip_id < num_mips());
    return header_ptr()->mips[mip_id].size_bytes;
}


auto TextureFile::mip_bytes(size_t mip_id) noexcept
    -> Span<ubyte>
{
    return { mip_bytes_ptr(mip_id), mip_size_bytes(mip_id) };
}


auto TextureFile::mip_bytes(size_t mip_id) const noexcept
    -> Span<const ubyte>
{
    return { mip_bytes_ptr(mip_id), mip_size_bytes(mip_id) };
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
        .preamble     = ResourcePreamble::create(resource_type, self_uuid),
        .num_channels = args.num_channels,
        .colorspace   = args.colorspace,
        ._reserved0   = {},
        .num_mips     = uint8_t(num_mips),
        .mips         = {}, // NOTE: Zero-init here. Fill later.
    };

    // Populate spans. From lowres MIPs to hires.
    size_t current_offset{ sizeof(Header) };
    for (size_t mip_id{ num_mips }; mip_id --> 0;) {
        MIPSpan&       span = header.mips   [mip_id];
        const MIPSpec& spec = args.mip_specs[mip_id];

        span.offset_bytes  = current_offset;
        span.size_bytes    = spec.size_bytes;
        span.width_pixels  = spec.width_pixels;
        span.height_pixels = spec.height_pixels;
        span.format        = spec.format;

        current_offset += span.size_bytes;
    }

    write_header_to(file.mapping_, header);

    return file;
}


auto TextureFile::open(MappedRegion mapped_region)
    -> TextureFile
{
    TextureFile file{ MOVE(mapped_region) };

    const size_t file_size = file.size_bytes();
    throw_if_too_small_for_header<Header>(file_size);

    Header& header = *file.header_ptr();

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
        if (to_underlying(file.format(mip_id)) >= enum_size<StorageFormat>()) {
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
