#pragma once
#include "CategoryCasts.hpp"
#include "RuntimeError.hpp"
#include "Region.hpp"
#include "Skeleton.hpp"
#include "Math.hpp"
#include "UUID.hpp"
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


using boost::interprocess::mapped_region;




class SkeletonFile {
public:
    struct Header {
        uint64_t _reserved0; // Reserved space. Maybe version or magic info.
        uint16_t _reserved1; // Reserved space. Maybe format or type?
        uint16_t num_joints; // Number of joints.
        uint32_t _padding0;  // Explicit alignment padding.
    };

    struct Args {
        uint16_t num_joints;
    };

    // Calculate the number of bytes required for creation of
    // the file with the specified arguments.
    static auto required_size(const Args& args) noexcept
        -> size_t;

    [[nodiscard]]
    static auto create_in(mapped_region mapped_region, const Args& args)
        -> SkeletonFile;

    [[nodiscard]]
    static auto open(mapped_region mapped_region)
        -> SkeletonFile;

    auto size_bytes() const noexcept -> size_t { return mapping_.get_size(); }
    auto num_joints() const noexcept -> uint16_t;
    auto joints()       noexcept -> std::span<Joint>;
    auto joints() const noexcept -> std::span<const Joint>;
    auto joint_names()       noexcept -> std::span<ResourceName>;
    auto joint_names() const noexcept -> std::span<const ResourceName>;

private:
    SkeletonFile(boost::interprocess::mapped_region mapping);
    boost::interprocess::mapped_region mapping_;

    auto header_ptr() const noexcept -> Header*;

    auto joints_ptr()      const noexcept -> Joint*;
    auto joint_names_ptr() const noexcept -> ResourceName*;
};



/*
struct JointSpan {
    uint32_t offset_bytes;
    uint32_t size_bytes;
};

struct Keyframes {
    uint32_t _reserved0;
    uint32_t num_pos_keys;
    uint32_t num_rot_keys;
    uint32_t num_sca_keys;
    KeyVec3  pos_keys[num_pos_keys];
    KeyQuat  rot_keys[num_rot_keys];
    KeyVec3  sca_keys[num_sca_keys];
};

struct AnimationFile {
    uint64_t  _reserved0;
    UUID      skeleton;
    float     duration_s;
    uint16_t  _reserved1;
    uint16_t  num_joints;
    JointSpan joints[num_joints];
    Keyframes keyframes[num_joints]; // Variable length each.
};

NOTE: This file layout requires double indirection to parse
the keyframes, reading the header alone is not enough.
*/
class AnimationFile {
public:
    struct JointSpan {
        uint32_t offset_bytes; // Offset at which keyframes of a particular joint are located.
        uint32_t size_bytes;   // Size for sanity.
    };

    struct KeyframesHeader {
        uint32_t _reserved0;
        uint32_t num_pos_keys;
        uint32_t num_rot_keys;
        uint32_t num_sca_keys;
    };

    struct KeyVec3 {
        float time_s;
        vec3  value;
    };

    struct KeyQuat {
        float time_s;
        quat  value;
    };

    struct Header {
        uint64_t  _reserved0;
        UUID      skeleton;
        float     duration_s;
        uint16_t  _reserved1;
        uint16_t  num_joints;
    };

    struct KeySpec {
        uint32_t num_pos_keys;
        uint32_t num_rot_keys;
        uint32_t num_sca_keys;
    };

    struct Args {
        std::span<const KeySpec> key_specs; // Per-joint.
    };

    static auto required_size(const Args& args) noexcept
        -> size_t;

    [[nodiscard]]
    static auto create_in(mapped_region mapped_region, const Args& args)
        -> AnimationFile;

    [[nodiscard]]
    static auto open(mapped_region mapped_region)
        -> AnimationFile;

    auto size_bytes() const noexcept -> size_t { return mapping_.get_size(); }

    auto skeleton_uuid()       noexcept -> UUID&;
    auto skeleton_uuid() const noexcept -> const UUID&;

    auto duration_s()       noexcept -> float&;
    auto duration_s() const noexcept -> const float&;

    auto num_joints() const noexcept -> uint16_t;

    auto num_pos_keys(size_t joint_id) const noexcept -> uint32_t;
    auto num_rot_keys(size_t joint_id) const noexcept -> uint32_t;
    auto num_sca_keys(size_t joint_id) const noexcept -> uint32_t;

    auto pos_keys(size_t joint_id)       noexcept -> std::span<KeyVec3>;
    auto pos_keys(size_t joint_id) const noexcept -> std::span<const KeyVec3>;
    auto rot_keys(size_t joint_id)       noexcept -> std::span<KeyQuat>;
    auto rot_keys(size_t joint_id) const noexcept -> std::span<const KeyQuat>;
    auto sca_keys(size_t joint_id)       noexcept -> std::span<KeyVec3>;
    auto sca_keys(size_t joint_id) const noexcept -> std::span<const KeyVec3>;


private:
    AnimationFile(boost::interprocess::mapped_region mapping);
    boost::interprocess::mapped_region mapping_;

    auto header_ptr() const noexcept -> Header*;

    auto joint_span_ptr(size_t joint_id) const noexcept -> JointSpan*;

    // Total size of KeyframesHeader + all Keys for a joint with `spec`.
    static auto keyframes_size(const KeySpec& spec) noexcept -> size_t;
    auto keyframes_ptr(size_t joint_id) const noexcept -> KeyframesHeader*;

    auto pos_keys_ptr(size_t joint_id) const noexcept -> KeyVec3*;
    auto rot_keys_ptr(size_t joint_id) const noexcept -> KeyQuat*;
    auto sca_keys_ptr(size_t joint_id) const noexcept -> KeyVec3*;

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

    struct Args {
        VertexLayout             layout;
        std::span<const LODSpec> lod_specs; // Up-to max_lods.
    };

    static auto required_size(const Args& args) noexcept
        -> size_t;

    [[nodiscard]]
    static auto create_in(mapped_region mapped_region, const Args& args)
        -> MeshFile;

    [[nodiscard]]
    static auto open(mapped_region mapped_region)
        -> MeshFile;

    auto size_bytes() const noexcept
        -> size_t
    {
        return mapping_.get_size();
    }

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

    struct Args {
        StorageFormat            format;
        std::span<const MIPSpec> mip_specs; // Up-to max_mips.
    };

    static auto required_size(const Args& args) noexcept
        -> size_t;

    [[nodiscard]]
    static auto create_in(mapped_region mapped_region, const Args& args)
        -> TextureFile;

    [[nodiscard]]
    static auto open(mapped_region mapped_region)
        -> TextureFile;

    auto size_bytes() const noexcept
        -> size_t
    {
        return mapping_.get_size();
    }

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


private:
    TextureFile(boost::interprocess::mapped_region mapping);
    boost::interprocess::mapped_region mapping_;

    auto header_ptr() const noexcept -> Header*;

    auto mip_bytes_ptr(size_t mip_id) const noexcept -> std::byte*;
};



} // namespace josh
