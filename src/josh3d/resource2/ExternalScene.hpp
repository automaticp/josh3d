#pragma once
#include "AABB.hpp"
#include "Asset.hpp"
#include "Common.hpp"
#include "ElementHelpers.hpp"
#include "Elements.hpp"
#include "EnumUtils.hpp"
#include "GLAPICommonTypes.hpp"
#include "Ranges.hpp"
#include "Scalars.hpp"
#include "StringHash.hpp"
#include "Transform.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <entt/entity/fwd.hpp>
#include <functional>
#include <memory>


/*
NOTE: "esr" stands for External Scene Representation.
*/
namespace josh::esr {


/*
Intermediate scene representation serving as a bridge between
importers/exporters and the destination representation.

The actual data: mesh attributes, images, animations, etc. is not stored
directly inside but only referenced through pointer-like views. That means
that the original source of the data should be kept alive for as long as
the access to the views is needed.

The scene takes the form and conventions of a "registry". All references
are stored as IDs and not through pointers on indices (or, god forbid, names).
The names are not guaranteed to be unique in any way, they are purely informative
and should not be used to identify objects.

The registry serves as a storage for all components of the scene.
This includes resources that have no presence in the scene directly
like textures, materials, animations, etc.

IDs are based on the ECS entity type. As opposed to a normal ECS registry, objects
associated with each IDs are typed according to their respective ID. Objects with
a Mesh component will have MeshID, objects with Animation component will have an
AnimationID, etc.

A single object will not have multiple components that will identify it as different
IDs, with the exception of the EntityID that serves to unify all objects that have
an actual presence in the scene-graph: meshes, cameras, lights.

Each importer has to construct a representation according to these conventions
so that each consumer or exporter can read them out, and vice-versa.

This is motivated by the need to unify the glTF and Assimp formats so that
the common parsing code would not have to be rewritten for everything.
Additionally, Assimp format is such a mess that working with it directly
is a major PITA.
*/
struct ExternalScene;


/*
Scene reference/ID vocabulary.
*/

using Registry         = entt::registry; // Underlying registry type.
using ID               = entt::entity;   // Any object in the representation.
using SceneID          = ID; // The container of root nodes identifying a "scene".
using MeshID           = ID; // Any mesh entity, possibly with bones.
using LightID          = ID; // Any light entity.
using CameraID         = ID; // Any camera entity.
using NodeID           = ID; // Any node of the scene-graph. Can reference, but is *not* an Entity.
using EntityID         = ID; // Any entity that is associated with a scene-graph node: Mesh, Light or Camera.
using ImageID          = ID; // Any image resource.
using TextureID        = ID; // Any texture (image + sampling params) as used by a material.
using MaterialID       = ID; // Any material (textures + params).
using SkinID           = ID; // Any skin (skeleton), possibly orphaned from meshes.
using AnimationID      = ID; // Any animation. Possibly mixed combination of different kinds.
using SkinAnimationID  = ID; // Animation data for this particular kind.
using NodeAnimationID  = ID; // Animation data for this particular kind.
using MorphAnimationID = ID; // Animation data for this particular kind.


/*
Special value used to identify lack of a referenced object.
It is *the* value implied when referred to as "null" or "no" id.
*/
constexpr ID null_id = entt::null;


/*
Vocabulary for containers used in ESR.
*/

using string = String;

template<typename Key, typename Value>
using map = HashMap<Key, Value>;

template<typename Key>
using set = HashSet<Key>;

template<typename Value>
using string_map = HashMap<string, Value, string_hash, std::equal_to<>>;

template<typename T>
using vector = SmallVector<T, 1>;

template<typename T>
using span = Span<T>;

/*
Scene objects and their components are defined below.
*/

/*
To follow along with glTF, there's multi-scene support.
Confusingly, ExternalSceneRepr can contain multiple scenes.
*/
struct Scene
{
    string         name;
    vector<NodeID> root_node_ids; // A single scene can contain multiple roots.
};

struct Node
{
    string           name;
    vector<EntityID> entities;   // List of associated entities, or empty if none.
    Transform        transform;  // Parent-to-Local transform of this node.
    NodeID           parent_id;  // Parent of the node.
    NodeID           child0_id;  // First child of the node.
    NodeID           sibling_id; // Next sibling of the node.
};

struct Light
{
    // TODO:
};

struct Camera
{
    // TODO:
};

/*
Singular mesh primitive. Unlike glTF definition as a collection of primitives.
*/
struct Mesh
{
    string         name;
    AttributeViews attributes;
    bool           is_skinned; // Could be skinned even if it does not refer to a skin_id in this scene.
    SkinID         skin_id;    // Referenced skin, if any.
};

// TODO: This is for identifying channels in a view.
// FIXME: Not used, remove?
enum class ChannelMask : u8
{
    R = (1 << 0),
    G = (1 << 1),
    B = (1 << 2),
    A = (1 << 3),
};
JOSH3D_DEFINE_ENUM_BITSET_OPERATORS(ChannelMask);

constexpr auto num_channels(ChannelMask mask) noexcept
    -> usize
{
    return std::popcount(to_underlying(mask));
}

// TODO: What should this be exactly?
struct ImageView
{
    ElementsView data;    // If not encoded, each element corresponds to an image "pixel". If encoded, this is just a byte stream.
    u32          width;   // Width in pixels. Might be 0 if encoded.
    u32          height;  // Height in pixels. Might be 0 if encoded.
    bool         encoded; // If encoded, the data needs to be decoded from some common format (ex. PNG, JPEG). The data.element will likely be just a byte stream u8vec1. This flag is used to differentiate it from a normal single-channel image.
    constexpr explicit operator bool() const noexcept { return bool(data); }
};

struct Image
{
    string    path;     // Relative path to source file on disk if not embedded, unspecified if it is.
    ImageView embedded; // Embedded image view. Optional, will be null if no data is embedded.
};

/*
As opposed to glTF, this isn't a separate object, but just a member field.
I don't think I ever had a reason to instance samplers. This state packs into like 2 bytes.
*/
struct SamplerInfo
{
    MinFilter min_filter = MinFilter::Linear;
    MagFilter mag_filter = MagFilter::Linear;
    Wrap      wrap_s     = Wrap::Repeat;
    Wrap      wrap_t     = Wrap::Repeat;
};

struct Texture
{
    string      name;
    ImageID     image_id;     // Referenced image.
    u8          channel0;     // First channel in the image data. The number of channels depends on the material slot.
    SamplerInfo sampler_info; // Sampler info. As data, not as a separate object.
};

enum class AlphaMethod : u8
{
    None,  // No alpha operations applied. Effectively opaque.
    Test,  // Test if above a threshold.
    Blend, // Blend based on the alpha value.
};
JOSH3D_DEFINE_ENUM_EXTRAS(AlphaMethod, None, Test, Blend);

/*
This is a bit of a soup of most things you could possibly want.
Not every texture type is currently supported for rendering, however.

NOTE: We split the base color into its RGB and Alpha parts.
We also split the MetallicRoughness into two textures.
This is better for compatibility with various formats.
Note that the data could still come from the same image,
the relevant channels will just be sliced after decoding.
*/
struct Material
{
    string      name;
    TextureID   albedo_id         = null_id;    // Surface albedo (aka. RGB of Base Color).
    vec3        albedo_factor     = vec3(1.0f); // Per-channel RGB multiplier applied to albedo.
    TextureID   alpha_id          = null_id;    // Single-channel transparency map.
    float       alpha_factor      = 1.0f;
    AlphaMethod alpha_method      = AlphaMethod::None;
    bool        double_sided      = false;      // Whether to enable backface culling. Would normally be true if `alpha_method` is not None.
    float       alpha_threshold   = 0.0f;       // Draw if `alpha > threshold`. Only considered if `alpha_method` is Test.
    TextureID   metallic_id       = null_id;    // PBR metallicity map.
    float       metallic_factor   = 1.0f;       // Additional factor to multiply metallicity by.
    TextureID   roughness_id      = null_id;    // PBR roughness map.
    float       roughness_factor  = 1.0f;       // Additional factor to multiply roughness by.
    TextureID   specular_color_id = null_id;    // Some "specular" map.
    vec3        specular_color_factor = vec3(1.0f);
    TextureID   specular_id       = null_id;
    float       specular_factor   = 1.0f;
    TextureID   normal_id         = null_id;    // Tangent-space normal map.
    TextureID   emissive_id       = null_id;    // Color emission map.
    vec3        emissive_factor   = vec3(1.0f); // Per-channel [0, 1] RGB multiplier applied to emissive.
    float       emissive_strength = 1.0f;       // HDR multiplier [0, inf] for emissive. If using SI this is probably [W/sr/m^2] (or nits?).
};


struct Joint
{
    string name;
    mat4   inv_bind;
    u32    parent_idx;  // Index in the `joints` array or -1 if no parent.
    u32    child0_idx;  // Index of the first child or -1 if no children.
    u32    sibling_idx; // Next sibling index of the same parent or -1 if last sibling.
    NodeID node_id;
};

/*
aka. Skeleton.
*/
struct Skin
{
    string           name;
    vector<Joint>    joints;     // In pre-order. First is root.
    map<NodeID, u32> joint_idxs; // Joint indices in the `joints` array. This is useful in resolving animations, and possibly in other places.
};

/*
HMM: I'm not sure if this is particularly useful, most scenarious would be fine
with linear only, and if a sharp step/break is needed, it's probably best to
encode that per keyframe, not per animation track. Or, you know, just create
two disjoint keyframes that just have a very short gap between them.
*/
enum class MotionInterpolation : u8
{
    Linear,      // Good old lerp.
    Step,        // Use the value of the last keyframe. NOTE: This is not "Nearest", more like "Left" or "Floor".
    CubicSpline, // Yeah, uh-uh. I can't even parse the data for this currently.
};
JOSH3D_DEFINE_ENUM_EXTRAS(MotionInterpolation, Step, Linear, CubicSpline);

struct MotionChannel
{
    // TODO: Should `tps` really be defined per-channel?
    float               tps           = {}; // Ticks per second. Zero if unknown. Use `ticks[i] * (tps ? tps : default_tps)` to recover time in seconds.
    MotionInterpolation interpolation = MotionInterpolation::Linear;
    ElementsView        ticks         = {}; // Time in abstract "ticks" from animation start time point.
    ElementsView        values        = {}; // Channel values, type depends on the usage slot. `element_count` should be the same as in `ticks`.
    constexpr explicit operator bool() const noexcept { return bool(ticks); }
};

struct TRSMotion
{
    MotionChannel translation;
    MotionChannel rotation;
    MotionChannel scaling;
};

struct WeightMotion
{
    MotionChannel weights;
};

struct NodeAnimation
{
    string                 name;
    map<NodeID, TRSMotion> motions;
};

/*
NOTE: The `skin_id` is duplicated with the Animation, as this should
be possible to use as a standalone animation, with Animation serving
more as a "multi-animation" containter.
*/
struct SkinAnimation
{
    string              name;
    SkinID              skin_id;
    map<u32, TRSMotion> motions; // JointIndex -> Motion.
};

/*
TODO: We currently do not implement this and so the data
model in my head could be completely off.
*/
struct MorphAnimation
{
    string name;
    // TODO: Stuff.
    // Oh god, this is sparse...
    MeshID mesh_id;
};

/*
Animation is more of a collection of different animation kinds that
are supposed to be played together.
*/
struct Animation
{
    string                        name;
    vector<NodeAnimationID>       node_animations;  // It's unspecified what it means to have multiple animations affecting the same node. First one wins? Current formats don't have this.
    map<SkinID, SkinAnimationID>  skin_animations;  // Yes, multiple skins per animation, enjoy.
    map<MeshID, MorphAnimationID> morph_animations;
};

struct ExternalScene
    : Registry
{
    // NOTE: No member variables here, everything is accessed
    // through the registry interface instead.

    template<typename T>
    struct Created { ID id; T& component; };

    template<typename T>
    auto create_as(T&& component) -> Created<T>;
};


template<typename T>
auto ExternalScene::create_as(T&& component)
    -> Created<T>
{
    const ID id = create();
    T& component_ = emplace<T>(id, FORWARD(component));
    return { .id=id, .component=component_ };
}


} // namespace josh::esr
