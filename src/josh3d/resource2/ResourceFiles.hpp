#pragma once
#include "Common.hpp"
#include "AABB.hpp"
#include "CategoryCasts.hpp"
#include "DefaultResources.hpp"
#include "EnumUtils.hpp"
#include "FileMapping.hpp"
#include "RuntimeError.hpp"
#include "Region.hpp"
#include "Skeleton.hpp"
#include "Math.hpp"
#include "UUID.hpp"
#include "VertexStatic.hpp"
#include "VertexSkinned.hpp"
#include <cstddef>
#include <cstdint>


/*
This file provides memory-mapped interfaces to binary resource files.
It also, implicitly, defines the layout of the binary resource files.
*/
namespace josh {


/*
First 24 bytes of each binary resource file.
*/
struct alignas(8) ResourcePreamble {
    uint32_t     _magic;        // "josh".
    ResourceType resource_type; // Type of this resource.
    UUID         self_uuid;     // UUID of this resource.

    static auto create(ResourceType type, const UUID& self_uuid) noexcept
        -> ResourcePreamble;
};




struct ResourceName {
    static constexpr size_t max_length = 63;

    uint8_t length;
    char    name[max_length];

    // Construct from a string view.
    // Truncates if the string is longer than `max_length`.
    static auto from_view(StrView sv) noexcept
        -> ResourceName;

    // Construct from a null-terminated string.
    // Truncates if the string is longer than `max_length`.
    static auto from_cstr(const char* cstr) noexcept
        -> ResourceName;

    auto view() const noexcept -> StrView;
    operator StrView() const noexcept { return view(); }
};


namespace error {
/*
Contents of a resource file make no sense.
*/
class InvalidResourceFile : public RuntimeError {
public:
    static constexpr auto prefix = "Invalid Resource File: ";
    InvalidResourceFile(String msg)
        : InvalidResourceFile(prefix, MOVE(msg))
    {}
protected:
    InvalidResourceFile(const char* prefix, String msg)
        : RuntimeError(prefix, MOVE(msg))
    {}
};
} // namespace error


/*
Pattern (ImHex):

struct Preamble {
    char magic[4];
    u32  resource_type;
    u8   self_uuid[16];
};

struct Joint {
    float inv_bind[16];
    u32   parent_id;
};

struct ResourceName {
    u8     len;
    char   name[len];
    padding[63 - len];
};

struct SkeletonFile {
    Preamble     preamble;
    u16          _reserved0;
    u16          num_joints;
    padding      [4];
    Joint        joints[num_joints];
    ResourceName joint_names[num_joints];
};

SkeletonFile skeleton_file @ 0x0;
*/
class SkeletonFile {
public:
    static constexpr auto resource_type = RT::Skeleton;

    struct Header {
        ResourcePreamble preamble;
        uint16_t         _reserved0; // Reserved space. Maybe format or type?
        uint16_t         num_joints; // Number of joints.
        uint32_t         _padding0;  // Explicit alignment padding.
    };

    struct Args {
        uint16_t num_joints;
    };

    // Calculate the number of bytes required for creation of
    // the file with the specified arguments.
    static auto required_size(const Args& args) noexcept
        -> size_t;

    [[nodiscard]]
    static auto create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
        -> SkeletonFile;

    [[nodiscard]]
    static auto open(MappedRegion mapped_region)
        -> SkeletonFile;

    auto size_bytes() const noexcept -> size_t { return mapping_.get_size(); }
    auto num_joints() const noexcept -> uint16_t;
    auto joints()       noexcept -> Span<Joint>;
    auto joints() const noexcept -> Span<const Joint>;
    auto joint_names()       noexcept -> Span<ResourceName>;
    auto joint_names() const noexcept -> Span<const ResourceName>;

private:
    SkeletonFile(MappedRegion mapping);
    MappedRegion mapping_;

    auto header_ptr() const noexcept -> Header*;

    auto joints_ptr()      const noexcept -> Joint*;
    auto joint_names_ptr() const noexcept -> ResourceName*;
};



/*
NOTE: This file layout requires double indirection to parse
the keyframes, reading the header alone is not enough.

Pattern (ImHex):

struct Preamble {
    char magic[4];
    u32  resource_type;
    u8   self_uuid[16];
};

struct JointSpan {
    u32 offset_bytes;
    u32 size_bytes;
};

struct KeyframesHeader {
    u32 _reserved0;
    u32 num_pos_keys;
    u32 num_rot_keys;
    u32 num_sca_keys;
};

struct vec3 {
    float x, y, z;
};

struct quat {
    float w, x, y, z;
};

struct KeyVec3 {
    float time_s;
    vec3  value;
};

struct KeyQuat {
    float time_s;
    quat  value;
};

struct Keyframes {
    KeyframesHeader header;
    KeyVec3         pos_keys[header.num_pos_keys];
    KeyQuat         rot_keys[header.num_rot_keys];
    KeyVec3         sca_keys[header.num_sca_keys];
};

struct AnimationFile {
    Preamble  preamble;
    u8        skeleton_uuid[16];
    float     duration_s;
    u16       _reserved0;
    u16       num_joints;
    JointSpan joints[num_joints];
    Keyframes keyframes[num_joints];
};

AnimationFile anim_file @ 0x0;
*/
class AnimationFile {
public:
    static constexpr auto resource_type = RT::Animation;

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
        ResourcePreamble preamble;
        UUID             skeleton_uuid;
        float            duration_s;
        uint16_t         _reserved1;
        uint16_t         num_joints;
    };

    struct KeySpec {
        uint32_t num_pos_keys;
        uint32_t num_rot_keys;
        uint32_t num_sca_keys;
    };

    struct Args {
        Span<const KeySpec> key_specs; // Per-joint.
    };

    static auto required_size(const Args& args) noexcept
        -> size_t;

    [[nodiscard]]
    static auto create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
        -> AnimationFile;

    [[nodiscard]]
    static auto open(MappedRegion mapped_region)
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

    auto pos_keys(size_t joint_id)       noexcept -> Span<KeyVec3>;
    auto pos_keys(size_t joint_id) const noexcept -> Span<const KeyVec3>;
    auto rot_keys(size_t joint_id)       noexcept -> Span<KeyQuat>;
    auto rot_keys(size_t joint_id) const noexcept -> Span<const KeyQuat>;
    auto sca_keys(size_t joint_id)       noexcept -> Span<KeyVec3>;
    auto sca_keys(size_t joint_id) const noexcept -> Span<const KeyVec3>;


private:
    AnimationFile(MappedRegion mapping);
    MappedRegion mapping_;

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
    static constexpr auto resource_type = RT::Mesh;
    static constexpr size_t max_lods = 8;

    enum class VertexLayout : uint16_t {
        Static  = 0, // VertexStatic
        Skinned = 1, // VertexSkinned

        _count,
    };

    enum class Compression : uint16_t {
        None = 0,

        _count,
    };

    template<VertexLayout V>   struct layout_traits;
    template<typename VertexT> struct vertex_traits;

    struct LODSpan {
        // FIXME: The vertex and element data must be aligned if uncompressed.
        uint32_t    offset_bytes; // Offset into the file, where the vertex data starts.
        uint32_t    num_verts;    // Number of vertices encoded in the data.
        uint32_t    num_elems;    // Number of elements in the data.
        uint32_t    verts_bytes;  // Size of the vertex data in bytes. Always comes first.
        uint32_t    elems_bytes;  // Size of the element data in bytes. Always comes after vertex data.
        Compression compression;  // Vertex compression type. Per LOD.
        uint16_t    _reserved0;   // Layout per-LOD?
    };

    struct Header {
        ResourcePreamble preamble;
        UUID             skeleton;       // UUID of the associated skeleton, if the layout is Skinned.
        VertexLayout     layout;         // An enum identifying a specific vertex layout.
        uint16_t         num_lods;       // Number of LODs stored for this mesh (up-to 8).
        LocalAABB        aabb;           // AABB in mesh space. Largest of all LODs.
        LODSpan          lods[max_lods]; // LOD descriptors that point to data in file.
    };

    struct LODSpec {
        uint32_t    num_verts;
        uint32_t    num_elems;
        uint32_t    verts_bytes;
        uint32_t    elems_bytes;
        Compression compression;
        uint16_t    _reserved0;
    };

    struct Args {
        VertexLayout        layout;
        Span<const LODSpec> lod_specs; // Up-to max_lods.
    };

    static auto required_size(const Args& args) noexcept
        -> size_t;

    [[nodiscard]]
    static auto create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
        -> MeshFile;

    [[nodiscard]]
    static auto open(MappedRegion mapped_region)
        -> MeshFile;

    auto size_bytes() const noexcept -> size_t { return mapping_.get_size(); }

    auto skeleton_uuid()       noexcept -> UUID&;
    auto skeleton_uuid() const noexcept -> const UUID&;

    auto aabb()       noexcept -> LocalAABB&;
    auto aabb() const noexcept -> const LocalAABB&;

    auto layout()   const noexcept -> VertexLayout;
    auto num_lods() const noexcept -> uint16_t;

    auto lod_spec       (size_t lod_id) const noexcept -> LODSpec;
    auto num_verts      (size_t lod_id) const noexcept -> uint32_t;
    auto num_elems      (size_t lod_id) const noexcept -> uint32_t;
    auto lod_compression(size_t lod_id) const noexcept -> Compression;

    auto lod_verts_size_bytes(size_t lod_id) const noexcept -> uint32_t;
    auto lod_elems_size_bytes(size_t lod_id) const noexcept -> uint32_t;

    auto lod_verts_bytes(size_t lod_id)       noexcept -> Span<ubyte>;
    auto lod_verts_bytes(size_t lod_id) const noexcept -> Span<const ubyte>;
    auto lod_elems_bytes(size_t lod_id)       noexcept -> Span<ubyte>;
    auto lod_elems_bytes(size_t lod_id) const noexcept -> Span<const ubyte>;

private:
    MeshFile(MappedRegion mapping);
    MappedRegion mapping_;

    auto header_ptr() const noexcept -> Header*;

    auto lod_verts_ptr(size_t lod_id) const noexcept -> ubyte*;
    auto lod_elems_ptr(size_t lod_id) const noexcept -> ubyte*;

    static auto vert_size(VertexLayout layout) noexcept -> size_t;
};


template<> struct MeshFile::layout_traits<MeshFile::VertexLayout::Static>  { using type = VertexStatic;  };
template<> struct MeshFile::layout_traits<MeshFile::VertexLayout::Skinned> { using type = VertexSkinned; };
template<> struct MeshFile::vertex_traits<VertexStatic>  { static constexpr VertexLayout layout = VertexLayout::Static;  };
template<> struct MeshFile::vertex_traits<VertexSkinned> { static constexpr VertexLayout layout = VertexLayout::Skinned; };




/*
NOTE: MIP levels are placed such that the lower resolution MIPs
are stored *before* the higher resolution ones. Reading the header
will cache the entire page, making access to the low-resolution MIPs "free".
*/
class TextureFile {
public:
    static constexpr auto resource_type = RT::Texture;
    static constexpr size_t max_mips = 16;

    // TODO: This is not fully supported yet.
    enum class StorageFormat : uint16_t {
        RAW, // No compression. Directly streamable.
        PNG, // High compression. Needs decoding.
        BC7, // Low compression. Directly streamable.

        _count,
    };

    struct MIPSpan {
        uint32_t      offset_bytes;
        uint32_t      size_bytes;
        uint16_t      width_pixels;
        uint16_t      height_pixels;
        StorageFormat format;
        uint16_t      _reserved0;
    };

    struct Header {
        ResourcePreamble preamble;
        uint16_t         num_channels;
        uint16_t         num_mips;
        MIPSpan          mips[max_mips];
    };

    struct MIPSpec {
        uint32_t      size_bytes;
        uint16_t      width_pixels;
        uint16_t      height_pixels;
        StorageFormat format;
    };

    struct Args {
        uint16_t            num_channels;
        Span<const MIPSpec> mip_specs; // Up-to max_mips.
    };

    static auto required_size(const Args& args) noexcept
        -> size_t;

    [[nodiscard]]
    static auto create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
        -> TextureFile;

    [[nodiscard]]
    static auto open(MappedRegion mapped_region)
        -> TextureFile;

    auto size_bytes() const noexcept -> size_t { return mapping_.get_size(); }

    auto num_mips()     const noexcept -> uint16_t;
    auto num_channels() const noexcept -> uint16_t;

    auto mip_spec  (size_t mip_id) const noexcept -> MIPSpec;
    auto format    (size_t mip_id) const noexcept -> StorageFormat;
    auto resolution(size_t mip_id) const noexcept -> Size2I;

    auto mip_size_bytes(size_t mip_id) const noexcept -> uint32_t;

    auto mip_bytes(size_t mip_id)       noexcept -> Span<ubyte>;
    auto mip_bytes(size_t mip_id) const noexcept -> Span<const ubyte>;


private:
    TextureFile(MappedRegion mapping);
    MappedRegion mapping_;

    auto header_ptr() const noexcept -> Header*;

    auto mip_bytes_ptr(size_t mip_id) const noexcept -> ubyte*;
};


JOSH3D_DEFINE_ENUM_STRING(TextureFile::StorageFormat, RAW, PNG, BC7)


} // namespace josh
