#include "ResourceFiles.hpp"
#include "EnumUtils.hpp"
#include "Filesystem.hpp"
#include "CategoryCasts.hpp"
#include "ReadFile.hpp"
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fmt/core.h>
#include <fmt/std.h>
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


auto mapping_bytes(const bip::mapped_region& mapping) noexcept
    -> std::byte*
{
    return (std::byte*)mapping.get_address();
}


template<typename HeaderT>
void throw_if_too_small_for_header(size_t file_size, const Path& path) noexcept(false) {
    if (file_size < sizeof(HeaderT)) {
        throw error::InvalidResourceFile(fmt::format(
            "Resource file \"{}\" is too small to contain header information.", path));
    }
}


void throw_on_unexpected_size(size_t expected, size_t real, const Path& path) noexcept(false) {
    if (real != expected) {
        throw error::InvalidResourceFile(fmt::format(
            "Resource file \"{}\" has unexpected size. Expected {}, got {}.",  path, expected, real));
    }
}


template<typename HeaderT>
[[nodiscard]] auto open_file_mapping(const Path& path)
    -> bip::mapped_region
{
    // NOTE: Mapped regions persist even after the file_mapping was destroyed,
    // so we drop the file_mapping at the end of this scope.

    bip::file_mapping  file{ path.c_str(), bip::read_write };
    bip::mapped_region mapping{ file, bip::read_write };
    mapping.advise(bip::mapped_region::advice_sequential); // TODO: Useful?

    const size_t file_size = mapping.get_size();

    throw_if_too_small_for_header<HeaderT>(file_size, path);

    return mapping;
}


[[nodiscard]] auto create_file_mapping(const Path& path, size_t file_size)
    -> bip::mapped_region
{
    std::filebuf file = create_file(path, file_size);

    // NOTE: This actually writes to disk. Since the file_mapping opens by-path
    // again, we need to flush the "resize" before that, else we are going to
    // open an "empty" file. Mapping it would fail.
    file.pubsync();

    bip::file_mapping file_mapping{ path.c_str(), bip::read_write };
    bip::mapped_region mapping{ file_mapping, bip::read_write };
    mapping.advise(bip::mapped_region::advice_sequential);

    return mapping;
}


// Headers are always assumed to be at the very beginning of a mapping.
template<typename HeaderT>
void write_header_to(bip::mapped_region& mapping, const HeaderT& src) noexcept {
    std::memcpy(mapping_bytes(mapping), &src, sizeof(HeaderT));
    mapping.flush(0, sizeof(HeaderT));
}


} // namespace












SkeletonFile::SkeletonFile(bip::mapped_region mapping)
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
    -> std::span<Joint>
{
    return { joints_ptr(), num_joints() };
}


auto SkeletonFile::joints() const noexcept
    -> std::span<const Joint>
{
    return { joints_ptr(), num_joints() };
}


auto SkeletonFile::joint_names() noexcept
    -> std::span<ResourceName>
{
    return { joint_names_ptr(), num_joints() };
}


auto SkeletonFile::joint_names() const noexcept
    -> std::span<const ResourceName>
{
    return { joint_names_ptr(), num_joints() };
}


auto SkeletonFile::open(const Path& path)
    -> SkeletonFile
{
    SkeletonFile file{ open_file_mapping<Header>(path) };

    const size_t file_size =
        file.mapping_.get_size();

    const size_t expected_size =
        sizeof(Header) +
        sizeof(Joint) * file.num_joints() +
        sizeof(ResourceName) * file.num_joints();

    throw_on_unexpected_size(expected_size, file_size, path);

    return file;
}


auto SkeletonFile::create(const Path& path, uint16_t num_joints)
    -> SkeletonFile
{
    assert(num_joints <= Skeleton::max_joints);
    const size_t size_header = sizeof(Header);
    const size_t size_joints = sizeof(Joint) * num_joints;
    const size_t size_names  = sizeof(ResourceName) * num_joints;
    const size_t total_size  = size_header + size_joints + size_names;

    SkeletonFile file = { create_file_mapping(path, total_size) };

    const Header header{
        ._reserved0 = {},
        ._reserved1 = {},
        .num_joints = num_joints,
        ._padding0  = {},
    };

    write_header_to(file.mapping_, header);

    return file;
}










AnimationFile::AnimationFile(boost::interprocess::mapped_region mapping)
    : mapping_{ MOVE(mapping) }
{}


auto AnimationFile::header_ptr() const noexcept
    -> Header*
{
    return please_type_pun<Header>(mapping_.get_address());
}


auto AnimationFile::pos_keys_ptr() const noexcept
    -> KeyVec3*
{
    const size_t offset = sizeof(Header);
    return please_type_pun<KeyVec3>(mapping_bytes(mapping_) + offset);
}


auto AnimationFile::rot_keys_ptr() const noexcept
    -> KeyQuat*
{
    const size_t offset =
        sizeof(Header)  +
        sizeof(KeyVec3) * num_pos_keys();
    return please_type_pun<KeyQuat>(mapping_bytes(mapping_) + offset);
}


auto AnimationFile::sca_keys_ptr() const noexcept
    -> KeyVec3*
{
    const size_t offset =
        sizeof(Header)  +
        sizeof(KeyVec3) * num_pos_keys() +
        sizeof(KeyQuat) * num_rot_keys();
    return please_type_pun<KeyVec3>(mapping_bytes(mapping_) + offset);
}


auto AnimationFile::skeleton_uuid() noexcept
    -> UUID&
{
    return header_ptr()->skeleton;
}


auto AnimationFile::skeleton_uuid() const noexcept
    -> const UUID&
{
    return header_ptr()->skeleton;
}


auto AnimationFile::num_pos_keys() const noexcept
    -> uint32_t
{
    return header_ptr()->pos_keys_span.num_keys;
}


auto AnimationFile::num_rot_keys() const noexcept
    -> uint32_t
{
    return header_ptr()->rot_keys_span.num_keys;
}


auto AnimationFile::num_sca_keys() const noexcept
    -> uint32_t
{
    return header_ptr()->sca_keys_span.num_keys;
}


auto AnimationFile::pos_keys() noexcept
    -> std::span<KeyVec3>
{
    return { pos_keys_ptr(), num_pos_keys() };
}


auto AnimationFile::pos_keys() const noexcept
    -> std::span<const KeyVec3>
{
    return { pos_keys_ptr(), num_pos_keys() };
}


auto AnimationFile::rot_keys() noexcept
    -> std::span<KeyQuat>
{
    return { rot_keys_ptr(), num_rot_keys() };
}


auto AnimationFile::rot_keys() const noexcept
    -> std::span<const KeyQuat>
{
    return { rot_keys_ptr(), num_rot_keys() };
}


auto AnimationFile::sca_keys() noexcept
    -> std::span<KeyVec3>
{
    return { sca_keys_ptr(), num_sca_keys() };
}


auto AnimationFile::sca_keys() const noexcept
    -> std::span<const KeyVec3>
{
    return { sca_keys_ptr(), num_sca_keys() };
}


auto AnimationFile::open(const Path& path)
    -> AnimationFile
{
    AnimationFile file      = { open_file_mapping<Header>(path) };
    const size_t  file_size = file.mapping_.get_size();
    Header&       header    = *file.header_ptr();

    const size_t expected_size =
        sizeof(Header) +
        header.pos_keys_span.num_keys * sizeof(KeyVec3) +
        header.rot_keys_span.num_keys * sizeof(KeyQuat) +
        header.sca_keys_span.num_keys * sizeof(KeyVec3);

    throw_on_unexpected_size(expected_size, file_size, path);

    return file;
}


auto AnimationFile::create(
    const Path& path,
    uint32_t    num_pos_keys,
    uint32_t    num_rot_keys,
    uint32_t    num_sca_keys)
        -> AnimationFile
{
    const size_t size_header   = sizeof(Header);
    const size_t size_pos_keys = sizeof(KeyVec3) * num_pos_keys;
    const size_t size_rot_keys = sizeof(KeyQuat) * num_rot_keys;
    const size_t size_sca_keys = sizeof(KeyVec3) * num_sca_keys;
    const size_t total_size    = size_header + size_pos_keys + size_rot_keys + size_sca_keys;

    AnimationFile file = { create_file_mapping(path, total_size) };

    const KeySpan pos_keys_span{
        .byte_offset = uint32_t(size_header),
        .num_keys    = num_pos_keys,
    };

    const KeySpan rot_keys_span{
        .byte_offset = uint32_t(pos_keys_span.byte_offset + size_pos_keys),
        .num_keys    = num_rot_keys,
    };

    const KeySpan sca_keys_span{
        .byte_offset = uint32_t(rot_keys_span.byte_offset + size_rot_keys),
        .num_keys    = num_sca_keys
    };

    const Header header{
        ._reserved0    = {},
        .skeleton      = {},
        .pos_keys_span = pos_keys_span,
        .rot_keys_span = rot_keys_span,
        .sca_keys_span = sca_keys_span,
    };

    write_header_to(file.mapping_, header);

    return file;
}










MeshFile::MeshFile(boost::interprocess::mapped_region mapping)
    : mapping_{ MOVE(mapping) }
{}


auto MeshFile::header_ptr() const noexcept
    -> Header*
{
    return please_type_pun<Header>(mapping_.get_address());
}


using VertexLayout = MeshFile::VertexLayout;


template<VertexLayout V>
auto MeshFile::lod_verts_ptr(size_t lod_id) const noexcept
    -> typename layout_traits<V>::type*
{
    assert(lod_id < num_lods());
    assert(layout() == V);
    const LODSpan& span = header_ptr()->lods[lod_id];
    const size_t offset = span.offset_bytes;
    using vertex_type = layout_traits<V>::type;
    return please_type_pun<vertex_type>(mapping_bytes(mapping_) + offset);
}


auto MeshFile::lod_elems_ptr(size_t lod_id) const noexcept
    -> uint32_t*
{
    assert(lod_id < num_lods());
    const LODSpan& span = header_ptr()->lods[lod_id];
    // TODO: Any alignment issues here?
    const size_t offset = span.offset_bytes + span.verts_bytes;
    return please_type_pun<uint32_t>(mapping_bytes(mapping_) + offset);
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
    const uint32_t vert_size_ = vert_size(header_ptr()->layout);
    return header_ptr()->lods[lod_id].verts_bytes / vert_size_;
}


auto MeshFile::num_elems(size_t lod_id) const noexcept
    -> uint32_t
{
    assert(lod_id < num_lods());
    const uint32_t elem_size = sizeof(uint32_t);
    return header_ptr()->lods[lod_id].elems_bytes / elem_size;
}


auto MeshFile::lod_spec(size_t lod_id) const noexcept
    -> LODSpec
{
    assert(lod_id < num_lods());
    const LODSpan& span = header_ptr()->lods[lod_id];
    const uint32_t vert_size = [&]{
        switch (header_ptr()->layout) {
            using enum VertexLayout;
            case Static:  return sizeof(layout_traits<Static>::type);
            case Skinned: return sizeof(layout_traits<Skinned>::type);
            default: std::terminate();
        }
    }();
    const uint32_t elem_size = sizeof(uint32_t);

    return {
        .num_verts = span.verts_bytes / vert_size,
        .num_elems = span.elems_bytes / elem_size,
    };
}


template<VertexLayout V>
auto MeshFile::lod_verts(size_t lod_id) noexcept
    -> std::span<typename layout_traits<V>::type>
{
    return { lod_verts_ptr<V>(lod_id), num_verts(lod_id) };
}


template<VertexLayout V>
auto MeshFile::lod_verts(size_t lod_id) const noexcept
    -> std::span<const typename layout_traits<V>::type>
{
    return { lod_verts_ptr<V>(lod_id), num_verts(lod_id) };
}


template auto MeshFile::lod_verts<VertexLayout::Static>(size_t lod_id) noexcept
    -> std::span<typename layout_traits<VertexLayout::Static>::type>;


template auto MeshFile::lod_verts<VertexLayout::Skinned>(size_t lod_id) noexcept
    -> std::span<typename layout_traits<VertexLayout::Skinned>::type>;


template auto MeshFile::lod_verts<VertexLayout::Static>(size_t lod_id) const noexcept
    -> std::span<const typename layout_traits<VertexLayout::Static>::type>;


template auto MeshFile::lod_verts<VertexLayout::Skinned>(size_t lod_id) const noexcept
    -> std::span<const typename layout_traits<VertexLayout::Skinned>::type>;


auto MeshFile::lod_elems(size_t lod_id) noexcept
    -> std::span<uint32_t>
{
    return { lod_elems_ptr(lod_id), num_elems(lod_id) };
}


auto MeshFile::lod_elems(size_t lod_id) const noexcept
    -> std::span<const uint32_t>
{
    return { lod_elems_ptr(lod_id), num_elems(lod_id) };
}


auto MeshFile::open(const Path& path)
    -> MeshFile
{
    MeshFile     file      = { open_file_mapping<Header>(path) };
    const size_t file_size = file.mapping_.get_size();
    Header&      header    = *file.header_ptr();

    // Check layout type.
    const bool valid_layout =
        to_underlying(header.layout) < to_underlying(VertexLayout::_count);

    if (!valid_layout) {
        throw error::InvalidResourceFile(fmt::format("Mesh file \"{}\" has invalid layout.", path));
    }

    // Check lod limit.
    if (header.num_lods > max_lods || header.num_lods == 0) {
        throw error::InvalidResourceFile(fmt::format("Mesh file \"{}\" specifies invalid number of LODs.", path));
    }

    // Check size.
    // Also check that each vertex bytesize is multiple of sizeof(VertexT).
    size_t expected_size = sizeof(Header);
    for (size_t lod_id{ 0 }; lod_id < header.num_lods; ++lod_id) {
        const size_t verts_bytes = header.lods[lod_id].verts_bytes;
        const size_t elems_bytes = header.lods[lod_id].elems_bytes;
        if (verts_bytes % vert_size(header.layout)) {
            throw error::InvalidResourceFile("Mesh file \"{}\" contains invalid vertex data.");
        }
        expected_size += verts_bytes;
        expected_size += elems_bytes;
    }

    throw_on_unexpected_size(expected_size, file_size, path);

    return file;
}


auto MeshFile::create(
    const Path&              path,
    VertexLayout             layout,
    std::span<const LODSpec> lod_specs)
        -> MeshFile
{
    const size_t num_lods = lod_specs.size();
    assert(num_lods <= max_lods);
    assert(num_lods > 0);

    const size_t vert_size = MeshFile::vert_size(layout);
    const size_t elem_size = sizeof(uint32_t);

    Header header{
        ._reserved0 = {},
        .skeleton   = {},
        .layout     = layout,
        .num_lods   = uint16_t(num_lods),
        .lods       = {}, // NOTE: Zero-init here. Fill later.
    };

    // Populate spans. From lowres LODs to hires.
    // NOTE: Uh, sorry for the "goes-to operator", it actually works here...
    size_t current_offset{ sizeof(Header) };
    for (size_t lod_id{ num_lods }; lod_id --> 0;) {
        LODSpan&       span = header.lods[lod_id];
        const LODSpec& spec = lod_specs  [lod_id];

        span.offset_bytes = current_offset;
        span.verts_bytes  = spec.num_verts * vert_size;
        span.elems_bytes  = spec.num_elems * elem_size;

        current_offset += span.verts_bytes + span.elems_bytes;
    }

    const size_t total_size = current_offset;

    MeshFile file = { create_file_mapping(path, total_size) };

    write_header_to(file.mapping_, header);

    return file;
}












TextureFile::TextureFile(boost::interprocess::mapped_region mapping)
    : mapping_{ MOVE(mapping) }
{}


auto TextureFile::header_ptr() const noexcept
    -> Header*
{
    return please_type_pun<Header>(mapping_.get_address());
}


using StorageFormat = TextureFile::StorageFormat;


auto TextureFile::mip_bytes_ptr(size_t mip_id) const noexcept
    -> std::byte*
{
    assert(mip_id < num_mips());
    const MIPSpan& span = header_ptr()->mips[mip_id];
    const size_t offset = span.offset_bytes;
    return mapping_bytes(mapping_) + offset;
}


auto TextureFile::format() const noexcept
    -> StorageFormat
{
    return header_ptr()->format;
}


auto TextureFile::num_mips() const noexcept
    -> uint16_t
{
    return header_ptr()->num_mips;
}


auto TextureFile::mip_spec(size_t mip_id) const noexcept
    -> MIPSpec
{
    assert(mip_id < num_mips());
    const MIPSpan& span       = header_ptr()->mips[mip_id];
    return {
        .size_bytes    = span.size_bytes,
        .width_pixels  = span.width_pixels,
        .height_pixels = span.height_pixels,
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
    -> std::span<std::byte>
{
    return { mip_bytes_ptr(mip_id), mip_size_bytes(mip_id) };
}


auto TextureFile::mip_bytes(size_t mip_id) const noexcept
    -> std::span<const std::byte>
{
    return { mip_bytes_ptr(mip_id), mip_size_bytes(mip_id) };
}


auto TextureFile::open(const Path& path)
    -> TextureFile
{
    TextureFile  file      = { open_file_mapping<Header>(path) };
    const size_t file_size = file.mapping_.get_size();
    Header&      header    = *file.header_ptr();

    // Check storage format.
    const bool valid_format =
        to_underlying(header.format) < to_underlying(StorageFormat::_count);

    if (!valid_format) {
        throw error::InvalidResourceFile(fmt::format("Texture file \"{}\" has invalid format.", path));
    }

    // Check mip limit.
    if (header.num_mips > max_mips || header.num_mips == 0) {
        throw error::InvalidResourceFile(fmt::format("Texture file \"{}\" specifies invalid number of MIPs.", path));
    }

    // Check size.
    size_t expected_size = sizeof(Header);
    for (size_t mip_id{ 0 }; mip_id < header.num_mips; ++mip_id) {
        const size_t size_bytes = header.mips[mip_id].size_bytes;
        expected_size += size_bytes;
    }

    throw_on_unexpected_size(expected_size, file_size, path);

    return file;
}


auto TextureFile::create(
    const Path&              path,
    StorageFormat            format,
    std::span<const MIPSpec> mip_specs)
        -> TextureFile
{
    const size_t num_mips = mip_specs.size();
    assert(num_mips <= max_mips);
    assert(num_mips > 0);

    Header header{
        ._reserved0 = {},
        .format     = format,
        .num_mips   = uint16_t(num_mips),
        .mips       = {}, // NOTE: Zero-init here. Fill later.
    };

    // Populate spans. From lowres MIPs to hires.
    size_t current_offset{ sizeof(Header) };
    for (size_t mip_id{ num_mips }; mip_id --> 0;) {
        MIPSpan&       span = header.mips[mip_id];
        const MIPSpec& spec = mip_specs  [mip_id];

        span.offset_bytes  = current_offset;
        span.size_bytes    = spec.size_bytes;
        span.width_pixels  = spec.width_pixels;
        span.height_pixels = spec.height_pixels;

        current_offset += span.size_bytes;
    }

    const size_t total_size = current_offset;

    TextureFile file = { create_file_mapping(path, total_size) };

    write_header_to(file.mapping_, header);

    return file;
}




} // namespace josh
