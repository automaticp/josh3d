#pragma once
#include "EnumUtils.hpp"
#include "FileMapping.hpp"
#include "ResourceFiles.hpp"
#include "DefaultResources.hpp"
#include "VertexSkinned.hpp"
#include <cstdint>


/*
This file provides memory-mapped interfaces to binary resource files
used for storing the default resources on disk.

It also, implicitly, defines the layout of the binary resource files.

NOTE: The interfaces expose references to memory mapped files, meaning
that the file contents can be modified directy. For most modifications,
however, the file would have to be resized, so they cannot just be done
inplace. In that case a new file would have to be allocated.

Use common sense to distinguish which fields are "mutable" after creation.
Encapsulating this fully creates too much boilerplate overhead and impedes
fast prototyping.
*/
namespace josh {


/*
ImHex Pattern:

struct Joint {
    float inv_bind[16];
    u32   parent_id;
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
    static constexpr auto     file_type     = "SkeletonFile"_hs;
    static constexpr uint16_t version       = 0;
    static constexpr auto     resource_type = RT::Skeleton;

    struct Header {
        ResourcePreamble preamble;
        uint16_t         _reserved0;
        uint16_t         num_joints;
        uint32_t         _reserved1;
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

    auto size_bytes()  const noexcept -> size_t { return mregion_.get_size(); }

    auto header()      const noexcept -> Header&;
    auto joints()      const noexcept -> Span<Joint>;
    auto joint_names() const noexcept -> Span<ResourceName>;

private:
    SkeletonFile(MappedRegion mregion) : mregion_(MOVE(mregion)) {}
    MappedRegion mregion_;
};




/*
NOTE: This file layout requires double indirection to parse
the keyframes, reading the header alone is not enough.

ImHex Pattern:

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
    static constexpr auto     file_type     = "AnimationFile"_hs;
    static constexpr uint16_t version       = 0;
    static constexpr auto     resource_type = RT::Animation;

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

    auto size_bytes() const noexcept -> size_t { return mregion_.get_size(); }

    auto header() const noexcept -> Header&;

    auto joint_span      (size_t joint_id) const noexcept -> JointSpan&;
    auto keyframes_header(size_t joint_id) const noexcept -> KeyframesHeader&;
    auto pos_keys        (size_t joint_id) const noexcept -> Span<KeyVec3>;
    auto rot_keys        (size_t joint_id) const noexcept -> Span<KeyQuat>;
    auto sca_keys        (size_t joint_id) const noexcept -> Span<KeyVec3>;

private:
    AnimationFile(MappedRegion mregion) : mregion_(MOVE(mregion)) {}
    MappedRegion mregion_;
};




/*
NOTE: LOD levels are placed such that the lower resolution LODs
are stored *before* the higher resolution ones. Reading the header
will cache the entire page, making access to the low-res LODs "free".
This is useful for incremental streaming.

Pattern:

struct LODData {
    u8 verts[header.lods[i].verts_size_bytes]; // Aligned to alignof(vertex_type).
    u8 elems[header.lods[i].elems_size_bytes]; // Aligned to alignof(element_type).
};

struct StaticMeshFile {
    Header  header;
    LODData lod_data[header.num_lods];
};
*/
class StaticMeshFile {
public:
    static constexpr auto     file_type     = "StaticMeshFile"_hs;
    static constexpr uint16_t version       = 0;
    static constexpr auto     resource_type = RT::StaticMesh;
    static constexpr size_t   max_lods      = 8;
    using vertex_type  = VertexStatic;
    using element_type = uint32_t;

    struct LODSpan {
        uint32_t num_verts;          // Number of vertices encoded in the data.
        uint32_t num_elems;          // Number of elements in the data.
        uint32_t verts_offset_bytes; // Offset into the file, where the vertex data starts.
        uint32_t elems_offset_bytes; // Offset into the file, where the element data starts.
        uint32_t verts_size_bytes;   // Size of the vertex data in bytes.
        uint32_t elems_size_bytes;   // Size of the element data in bytes.
    };

    struct Header {
        ResourcePreamble preamble;       //
        uint16_t         _reserved0;     //
        uint8_t          num_lods;       // Number of LODs stored for this mesh (up-to 8).
        uint8_t          _reserved1;     //
        LocalAABB        aabb;           // AABB in mesh space. Largest of all LODs.
        uint32_t         _reserved2;     //
        LODSpan          lods[max_lods]; // LOD descriptors that point to data in file.
    };

    struct LODSpec {
        uint32_t num_verts;
        uint32_t num_elems;
        uint32_t verts_size_bytes;
        uint32_t elems_size_bytes;
    };

    struct Args {
        Span<const LODSpec> lod_specs; // Up-to max_lods.
    };

    static auto required_size(const Args& args) noexcept
        -> size_t;

    [[nodiscard]]
    static auto create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
        -> StaticMeshFile;

    [[nodiscard]]
    static auto open(MappedRegion mapped_region)
        -> StaticMeshFile;

    auto size_bytes() const noexcept -> size_t { return mregion_.get_size(); }

    auto header() const noexcept -> Header&;

    auto lod_span       (size_t lod_id) const noexcept -> LODSpan&;
    auto lod_verts_bytes(size_t lod_id) const noexcept -> Span<ubyte>;
    auto lod_elems_bytes(size_t lod_id) const noexcept -> Span<ubyte>;

private:
    StaticMeshFile(MappedRegion mregion) : mregion_(MOVE(mregion)) {}
    MappedRegion mregion_;
};


/*
NOTE: LOD levels are placed such that the lower resolution LODs
are stored *before* the higher resolution ones. Reading the header
will cache the entire page, making access to the low-res LODs "free".
This is useful for incremental streaming.

Pattern:

struct LODData {
    u8 verts[header.lods[i].verts_size_bytes]; // Aligned to alignof(vertex_type).
    u8 elems[header.lods[i].elems_size_bytes]; // Aligned to alignof(element_type).
};

struct StaticMeshFile {
    Header  header;
    LODData lod_data[header.num_lods];
};
*/
class SkinnedMeshFile {
public:
    static constexpr auto     file_type     = "SkinnedMeshFile"_hs;
    static constexpr uint16_t version       = 0;
    static constexpr auto     resource_type = RT::SkinnedMesh;
    static constexpr size_t   max_lods      = 8;
    using vertex_type  = VertexSkinned;
    using element_type = uint32_t;

    struct LODSpan {
        uint32_t num_verts;          // Number of vertices encoded in the data.
        uint32_t num_elems;          // Number of elements in the data.
        uint32_t verts_offset_bytes; // Offset into the file, where the vertex data starts.
        uint32_t elems_offset_bytes; // Offset into the file, where the element data starts.
        uint32_t verts_size_bytes;   // Size of the vertex data in bytes.
        uint32_t elems_size_bytes;   // Size of the element data in bytes.
    };

    struct Header {
        ResourcePreamble preamble;       //
        UUID             skeleton_uuid;  // UUID of the associated skeleton.
        uint16_t         _reserved0;     //
        uint8_t          num_lods;       // Number of LODs stored for this mesh (up-to 8).
        uint8_t          _reserved1;     //
        LocalAABB        aabb;           // AABB in mesh space. Largest of all LODs.
        uint32_t         _reserved2;     //
        LODSpan          lods[max_lods]; // LOD descriptors that point to data in file.
    };

    struct LODSpec {
        uint32_t num_verts;
        uint32_t num_elems;
        uint32_t verts_size_bytes;
        uint32_t elems_size_bytes;
    };

    struct Args {
        UUID                skeleton_uuid;
        Span<const LODSpec> lod_specs; // Up-to max_lods.
    };

    static auto required_size(const Args& args) noexcept
        -> size_t;

    [[nodiscard]]
    static auto create_in(MappedRegion mapped_region, UUID self_uuid, const Args& args)
        -> SkinnedMeshFile;

    [[nodiscard]]
    static auto open(MappedRegion mapped_region)
        -> SkinnedMeshFile;

    auto size_bytes() const noexcept -> size_t { return mregion_.get_size(); }

    auto header() const noexcept -> Header&;

    auto lod_span       (size_t lod_id) const noexcept -> LODSpan&;
    auto lod_verts_bytes(size_t lod_id) const noexcept -> Span<ubyte>;
    auto lod_elems_bytes(size_t lod_id) const noexcept -> Span<ubyte>;

private:
    SkinnedMeshFile(MappedRegion mregion) : mregion_(MOVE(mregion)) {}
    MappedRegion mregion_;
};




/*
NOTE: MIP levels are placed such that the lower resolution MIPs
are stored *before* the higher resolution ones. Reading the header
will cache the entire page, making access to the low-resolution MIPs "free".

Pattern:

struct MIPSpan {
    u32      offset_bytes;
    u32      size_bytes;
    u16      width_pixels;
    u16      height_pixels;
    Encoding encoding;
    u16      _reserved0;
};

struct MIPData {
    u8 bytes[mips[i].size_bytes];
};

struct TextureFile {
    Preamble   preamble;
    u8         num_channels;
    Colorspace colorspace;
    u8         _reserved0;
    u8         num_mips;
    u32        _reserved1;
    MIPSpan    mips[max_mips];

    MIPData    mip_data[num_mips];
};
*/
class TextureFile {
public:
    static constexpr auto     file_type     = "TextureFile"_hs;
    static constexpr uint16_t version       = 0;
    static constexpr auto     resource_type = RT::Texture;
    static constexpr size_t   max_mips      = 16;

    // TODO: This is not fully supported yet.
    enum class Encoding : uint8_t {
        RAW, // No compression. Directly streamable.
        PNG, // High compression. Needs decoding.
        BC7, // Low compression. Directly streamable.
    };

    enum class Colorspace : uint8_t {
        Linear,
        sRGB,
    };

    struct MIPSpan {
        uint32_t offset_bytes;
        uint32_t size_bytes;
        uint16_t width;  // In pixels.
        uint16_t height; // In pixels.
        Encoding encoding;
        uint8_t  _reserved0;
        uint16_t _reserved1;
    };

    struct Header {
        ResourcePreamble preamble;
        uint8_t          num_channels;
        Colorspace       colorspace;
        uint8_t          _reserved0;
        uint8_t          num_mips;
        uint32_t         _reserved1;
        MIPSpan          mips[max_mips];
    };

    struct MIPSpec {
        uint32_t size_bytes;
        uint16_t width_pixels;
        uint16_t height_pixels;
        Encoding encoding;
    };

    struct Args {
        uint8_t             num_channels;
        Colorspace          colorspace;
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

    auto size_bytes() const noexcept -> size_t { return mregion_.get_size(); }

    auto header() const noexcept -> Header&;

    auto mip_span (size_t mip_id) const noexcept -> MIPSpan&;
    auto mip_bytes(size_t mip_id) const noexcept -> Span<ubyte>;

private:
    TextureFile(MappedRegion mregion) : mregion_(MOVE(mregion)) {}
    MappedRegion mregion_;
};

JOSH3D_DEFINE_ENUM_EXTRAS(TextureFile::Encoding, RAW, PNG, BC7);
JOSH3D_DEFINE_ENUM_EXTRAS(TextureFile::Colorspace, Linear, sRGB);


} // namespace josh
