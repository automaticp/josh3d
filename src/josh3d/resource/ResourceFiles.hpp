#pragma once
#include "CategoryCasts.hpp"
#include "Filesystem.hpp"
#include "Region.hpp"
#include "Skeleton.hpp"
#include "Math.hpp"
#include "VertexPNUTB.hpp"
#include "VertexSkinned.hpp"
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstddef>
#include <cstdint>
#include <span>


/*
This file provides memory-mapped interfaces to binary resource files.
It also, implicitly, defines the layout of the binary resource files.
*/
namespace josh {


struct alignas(8) UUID {
    uint8_t bytes[16];
};


struct ResourceName {
    char name[64];
};


namespace error {
/*
Contents of a resource file make no sense.
*/
class InvalidResourceFile : public RuntimeError {
public:
    static constexpr auto prefix = "Invalid Resource File: ";
    InvalidResourceFile(std::string msg)
        : InvalidResourceFile(prefix, MOVE(msg))
    {}
protected:
    InvalidResourceFile(const char* prefix, std::string msg)
        : RuntimeError(prefix, MOVE(msg))
    {}
};
} // namespace error




class SkeletonFile {
public:
    struct Header {
        uint64_t _reserved0; // Reserved space. Maybe version or magic info.
        uint16_t _reserved1; // Reserved space. Maybe format or type?
        uint16_t num_joints; // Number of joints.
        uint32_t _padding0;  // Explicit alignment padding.
    };

    auto num_joints() const noexcept -> uint16_t;

    auto joints()       noexcept -> std::span<Joint>;
    auto joints() const noexcept -> std::span<const Joint>;

    auto joint_names()       noexcept -> std::span<ResourceName>;
    auto joint_names() const noexcept -> std::span<const ResourceName>;

    [[nodiscard]] static auto open(const Path& path) -> SkeletonFile;
    [[nodiscard]] static auto create(const Path& path, uint16_t num_joints) -> SkeletonFile;

private:
    SkeletonFile(boost::interprocess::mapped_region mapping);
    boost::interprocess::mapped_region mapping_;

    auto header_ptr() const noexcept -> Header*;

    auto joints_ptr()      const noexcept -> Joint*;
    auto joint_names_ptr() const noexcept -> ResourceName*;
};




class AnimationFile {
public:
    struct KeySpan {
        uint32_t byte_offset;
        uint32_t num_keys;
    };

    struct KeyVec3 {
        double   time;
        vec3     value;
        uint32_t _padding0;
    };

    struct KeyQuat {
        double time;
        quat   value;
    };

    struct Header {
        uint64_t _reserved0;
        UUID     skeleton;
        KeySpan  pos_keys_span;
        KeySpan  rot_keys_span;
        KeySpan  sca_keys_span;
    };

    auto skeleton_uuid()       noexcept -> UUID&;
    auto skeleton_uuid() const noexcept -> const UUID&;

    auto num_pos_keys() const noexcept -> uint32_t;
    auto num_rot_keys() const noexcept -> uint32_t;
    auto num_sca_keys() const noexcept -> uint32_t;

    auto pos_keys()       noexcept -> std::span<KeyVec3>;
    auto pos_keys() const noexcept -> std::span<const KeyVec3>;
    auto rot_keys()       noexcept -> std::span<KeyQuat>;
    auto rot_keys() const noexcept -> std::span<const KeyQuat>;
    auto sca_keys()       noexcept -> std::span<KeyVec3>;
    auto sca_keys() const noexcept -> std::span<const KeyVec3>;

    [[nodiscard]] static auto open(const Path& path) -> AnimationFile;
    [[nodiscard]] static auto create(
        const Path& path,
        uint32_t    num_pos_keys,
        uint32_t    num_rot_keys,
        uint32_t    num_sca_keys)
            -> AnimationFile;

private:
    AnimationFile(boost::interprocess::mapped_region mapping);
    boost::interprocess::mapped_region mapping_;

    auto header_ptr() const noexcept -> Header*;

    auto pos_keys_ptr() const noexcept -> KeyVec3*;
    auto rot_keys_ptr() const noexcept -> KeyQuat*;
    auto sca_keys_ptr() const noexcept -> KeyVec3*;
};




/*
NOTE: LOD levels are placed such that the lower resolution LODs
are stored *before* the higher resolution ones. Reading the header
will cache the entire page, making access to the low-resolution LODs "free".
This is useful for incremental streaming.
*/
class MeshFile {
public:
    static constexpr size_t max_lods = 8;

    enum class VertexLayout : uint16_t {
        Static  = 0, // VertexPNUTB
        Skinned = 1, // VertexSkinned

        _count,
    };

    template<VertexLayout V>   struct layout_traits;
    template<typename VertexT> struct vertex_traits;

    struct LODSpan {
        uint32_t offset_bytes; // Offset into the file, where the vertex data starts.
        uint32_t verts_bytes;  // Size of the vertex data in bytes. Always comes first.
        uint32_t elems_bytes;  // Size of the element data in bytes. Always comes after vertex data.
    };

    struct Header {
        uint64_t     _reserved0;     // Reserved space. Maybe version or magic info.
        UUID         skeleton;       // UUID of the associated skeleton, if the layout is Skinned.
        VertexLayout layout;         // An enum identifying a specific vertex layout.
        uint16_t     num_lods;       // Number of LODs stored for this mesh (up-to 8).
        LODSpan      lods[max_lods]; // LOD descriptors that point to data in file.
    };

    struct LODSpec {
        uint32_t num_verts;
        uint32_t num_elems;
    };

    // TODO: set/get to check if the layout is correct?
    auto skeleton_uuid() noexcept
        -> UUID&;

    auto skeleton_uuid() const noexcept
        -> const UUID&;

    auto layout() const noexcept
        -> VertexLayout;

    auto num_lods() const noexcept -> uint16_t;

    auto lod_spec (size_t lod_id) const noexcept -> LODSpec;
    auto num_verts(size_t lod_id) const noexcept -> uint32_t;
    auto num_elems(size_t lod_id) const noexcept -> uint32_t;

    template<VertexLayout V>
    auto lod_verts(size_t lod_id) noexcept
         -> std::span<typename layout_traits<V>::type>;

    template<VertexLayout V>
    auto lod_verts(size_t lod_id) const noexcept
        -> std::span<const typename layout_traits<V>::type>;

    auto lod_elems(size_t lod_id) noexcept
        -> std::span<uint32_t>;

    auto lod_elems(size_t lod_id) const noexcept
        -> std::span<const uint32_t>;


    [[nodiscard]]
    static auto open(const Path& path)
        -> MeshFile;

    [[nodiscard]]
    static auto create(
        const Path&              path,
        VertexLayout             layout,
        std::span<const LODSpec> lod_specs) // Up-to max_lods.
            -> MeshFile;

private:
    MeshFile(boost::interprocess::mapped_region mapping);
    boost::interprocess::mapped_region mapping_;

    auto header_ptr() const noexcept -> Header*;

    template<VertexLayout V>
    auto lod_verts_ptr(size_t lod_id) const noexcept -> typename layout_traits<V>::type*;
    auto lod_elems_ptr(size_t lod_id) const noexcept -> uint32_t*;

    static auto vert_size(VertexLayout layout) noexcept -> size_t;
};


template<> struct MeshFile::layout_traits<MeshFile::VertexLayout::Static>  { using type = VertexPNUTB;   };
template<> struct MeshFile::layout_traits<MeshFile::VertexLayout::Skinned> { using type = VertexSkinned; };
template<> struct MeshFile::vertex_traits<VertexPNUTB>   { static constexpr VertexLayout layout = VertexLayout::Static;  };
template<> struct MeshFile::vertex_traits<VertexSkinned> { static constexpr VertexLayout layout = VertexLayout::Skinned; };




/*
NOTE: MIP levels are placed such that the lower resolution MIPs
are stored *before* the higher resolution ones. Reading the header
will cache the entire page, making access to the low-resolution MIPs "free".
*/
class TextureFile {
public:
    static constexpr size_t max_mips = 16;

    // TODO: This is not really supported yet.
    enum class StorageFormat : uint16_t {
        BC7_RGB,  // Low compression. Directly streamable.
        BC7_RGBA, // Low compression. Directly streamable.
        PNG,      // High compression. Needs decoding.
        RAW,      // No compression. Directly streamable.

        _count,
    };

    struct MIPSpan {
        uint32_t offset_bytes;
        uint32_t size_bytes;
        uint16_t width_pixels;
        uint16_t height_pixels;
    };

    struct Header {
        uint64_t      _reserved0;
        StorageFormat format;
        uint16_t      num_mips;
        MIPSpan       mips[max_mips];
    };

    struct MIPSpec {
        uint32_t size_bytes;
        uint16_t width_pixels;
        uint16_t height_pixels;
    };

    auto format() const noexcept
        -> StorageFormat;

    auto num_mips() const noexcept
        -> uint16_t;

    auto mip_spec(size_t mip_id) const noexcept
        -> MIPSpec;

    auto resolution(size_t mip_id) const noexcept
        -> Size2I;

    auto mip_size_bytes(size_t mip_id) const noexcept
        -> uint32_t;

    auto mip_bytes(size_t mip_id) noexcept
        -> std::span<std::byte>;

    auto mip_bytes(size_t mip_id) const noexcept
        -> std::span<const std::byte>;

    [[nodiscard]]
    static auto open(const Path& path)
        -> TextureFile;

    [[nodiscard]]
    static auto create(
        const Path&              path,
        StorageFormat            format,
        std::span<const MIPSpec> mip_specs) // Up-to max_mips.
            -> TextureFile;

private:
    TextureFile(boost::interprocess::mapped_region mapping);
    boost::interprocess::mapped_region mapping_;

    auto header_ptr() const noexcept -> Header*;

    auto mip_bytes_ptr(size_t mip_id) const noexcept -> std::byte*;
};



} // namespace josh
