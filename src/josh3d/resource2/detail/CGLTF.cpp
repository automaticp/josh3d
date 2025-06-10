#include "CGLTF.hpp"
#include "AABB.hpp"
#include "Common.hpp"
#include "ContainerUtils.hpp"
#include "Elements.hpp"
#include "ExternalScene.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLTextures.hpp"
#include "ImageProperties.hpp"
#include "Processing.hpp"
#include "Ranges.hpp"
#include "Transform.hpp"
#include "VertexFormat.hpp"
#include <cgltf.h>
#include <glm/ext.hpp>


namespace josh::detail::cgltf {

auto to_transform(const cgltf_node& node) noexcept
    -> Transform
{
    Transform tf = {};
    if (node.has_matrix)
    {
        vec3 _skew;
        vec4 _perspective;
        glm::decompose(to_mat4(node.matrix), tf.scaling(), tf.orientation(), tf.position(), _skew, _perspective);
    }

    if (node.has_translation) tf.position()    = to_vec3(node.translation);
    if (node.has_rotation)    tf.orientation() = to_quat(node.rotation);
    if (node.has_scale)       tf.scaling()     = to_vec3(node.scale);

    return tf;
}

auto to_local_aabb(const cgltf_accessor& accessor) noexcept
    -> Optional<LocalAABB>
{
    auto grab_vec3 = [](const float* src_arr, Element src_element)
    {
        const ElementsView src = {
            .bytes         = src_arr,
            .element_count = 1,
            .stride        = u32(element_size(src_element)),
            .element       = src_element,
        };
        return copy_convert_one_element<vec3>(src, 0);
    };

    if (accessor.has_min and accessor.has_max)
        if (const Optional element = to_element(accessor.component_type, accessor.type, accessor.normalized))
            return LocalAABB{ grab_vec3(accessor.min, *element), grab_vec3(accessor.max, *element) };

    return nullopt;
}

auto to_sampler_info(const cgltf_sampler& _sampler) noexcept
    -> esr::SamplerInfo
{
    // NOTE: The integer values are directly convertible to their GL counterparts.
    return {
        .min_filter = MinFilter(_sampler.min_filter),
        .mag_filter = MagFilter(_sampler.mag_filter),
        .wrap_s     = Wrap(_sampler.wrap_s),
        .wrap_t     = Wrap(_sampler.wrap_t),
    };
}

auto to_motion_interpolation(cgltf_interpolation_type interp) noexcept
    -> esr::MotionInterpolation
{
    switch (interp)
    {
        case cgltf_interpolation_type_linear:       return esr::MotionInterpolation::Linear;
        case cgltf_interpolation_type_step:         return esr::MotionInterpolation::Step;
        case cgltf_interpolation_type_cubic_spline: return esr::MotionInterpolation::CubicSpline;
        default:                                    return esr::MotionInterpolation::Linear;
    }
}

namespace {

constexpr auto invalid_layout = ElementLayout(-1);
constexpr auto invalid_type   = ComponentType(-1);

auto to_component_type(cgltf_component_type _type, bool normalized)
    -> ComponentType
{
    const ComponentType type = eval%[&]{
        switch (_type)
        {
            using enum ComponentType;
            case cgltf_component_type_r_8:   return normalized ? i8norm  : i8;
            case cgltf_component_type_r_8u:  return normalized ? u8norm  : u8;
            case cgltf_component_type_r_16:  return normalized ? i16norm : i16;
            case cgltf_component_type_r_16u: return normalized ? u16norm : u16;
            // Uhh, there's no i32 type? Okay... Why?
            // case cgltf_component_type_r_32:  return ComponentType::i32;
            case cgltf_component_type_r_32u: return u32;
            case cgltf_component_type_r_32f: return f32;
            default:                         return invalid_type;
        }
    };
    return type;
}

auto to_element_layout(cgltf_type _layout)
    -> ElementLayout
{
    const ElementLayout layout = eval%[&]{
        switch (_layout)
        {
            using enum ElementLayout;
            case cgltf_type_scalar: return vec1;
            case cgltf_type_vec2:   return vec2;
            case cgltf_type_vec3:   return vec3;
            case cgltf_type_vec4:   return vec4;
            default:                return invalid_layout;
        }
    };
    return layout;
}

} // namespace

auto to_element(cgltf_component_type _type, cgltf_type _layout, bool normalized)
    -> Optional<Element>
{
    if (normalized)
    {
        const bool normalizable =
            _type == cgltf_component_type_r_8 or
            _type == cgltf_component_type_r_8u or
            _type == cgltf_component_type_r_16 or
            _type == cgltf_component_type_r_16u;

        if (not normalizable)
            return nullopt;
    }

    const ComponentType type   = to_component_type(_type, normalized);
    const ElementLayout layout = to_element_layout(_layout);

    // NOTE: Type and layout are bit-packed in Element, we check -1 before
    // packing, or else the values will become unrepresentable.
    if (layout == invalid_layout or type == invalid_type)
        return nullopt;

    return Element{ type, layout };
}

auto to_elements_view(const cgltf_accessor& accessor)
    -> ElementsView
{
    const Optional<Element> element = to_element(accessor.component_type, accessor.type);
    if (not element) return {};

    // NOTE: See cgltf_accessor_unpack_floats() or similar for how to
    // handle offsets, strides, and other stuff.

    if (not accessor.buffer_view) return {};
    const ubyte* bytes = cgltf_buffer_view_data(accessor.buffer_view);

    // Needs to be done, even though above already offsets by *buffer view* offset.
    bytes += accessor.offset;

    const usize element_count = accessor.count;
    const usize stride        = accessor.stride;

    return {
        .bytes         = bytes,
        .element_count = element_count,
        .stride        = u32(stride),
        .element       = *element,
    };
}

auto parse_primitive_attributes(
    const cgltf_primitive& _primitive,
    Optional<LocalAABB>*   aabb)
        -> esr::MeshAttributes
{
    constexpr usize N = 6;
    const cgltf_attribute* attributes[N] = {
        nullptr, // Positions
        nullptr, // UVs
        nullptr, // Normals
        nullptr, // Tangents
        nullptr, // Joint Indices
        nullptr, // Joint Weights
    };

    const auto _attributes = make_span(_primitive.attributes, _primitive.attributes_count);
    for (const cgltf_attribute& _attribute : _attributes)
    {
        const uindex i = eval%[&]{
            switch (_attribute.type)
            {
                case cgltf_attribute_type_position: return 0;
                case cgltf_attribute_type_texcoord: return 1;
                case cgltf_attribute_type_normal:   return 2;
                case cgltf_attribute_type_tangent:  return 3;
                case cgltf_attribute_type_joints:   return 4;
                case cgltf_attribute_type_weights:  return 5;
                default: return -1;
            }
        };

        if (i != uindex(-1) and _attribute.index == 0)
            attributes[i] = &_attribute;
    }

    const cgltf_accessor* indices = _primitive.indices;

    if (aabb)
    {
        if (attributes[0] and attributes[0]->data)
            *aabb = to_local_aabb(*attributes[0]->data);
        else
            *aabb = nullopt;
    }

    return {
        .indices   = indices       ? to_elements_view(*indices)             : ElementsView(),
        .positions = attributes[0] ? to_elements_view(*attributes[0]->data) : ElementsView(),
        .uvs       = attributes[1] ? to_elements_view(*attributes[1]->data) : ElementsView(),
        .normals   = attributes[2] ? to_elements_view(*attributes[2]->data) : ElementsView(),
        .tangents  = attributes[3] ? to_elements_view(*attributes[3]->data) : ElementsView(),
        .joint_ids = attributes[4] ? to_elements_view(*attributes[4]->data) : ElementsView(),
        .joint_ws  = attributes[5] ? to_elements_view(*attributes[5]->data) : ElementsView(),
    };
}

namespace {

/*
Will fill `skin.joints` in preorder, `skin.joint_idxs` and `_joint2preorder_idx`.
*/
void populate_joints_preorder(
    esr::Skin&                                      skin,
    const cgltf_node*                               _node,               // Current joint node, start from root.
    esr::map<const cgltf_node*, u32>&               _joint2preorder_idx, // Index in our esr::Skin::joints array.
    const cgltf_skin&                               _skin,               // Source skin. Maybe has inv_bind.
    const esr::map<const cgltf_node*, uindex>&      _joint2idx,          // Index in the cgltf_skin::joints array.
    const esr::map<const cgltf_node*, esr::NodeID>& _node2node_id)       // Map from node to associated NodeID, must be fully populated before.
{
    assert(_node);

    // Have to check that the child still belongs to the skeleton structure.
    // If not, we ignore the following subtree as it is not part of the skeleton.
    // Here the _joint2idx is used as a set, the actual index might not be initialized.
    u32* joint_idx_ptr = try_find_value(_joint2preorder_idx, _node);
    if (not joint_idx_ptr) return;

    const mat4 inv_bind = eval%[&]{
        if (_skin.inverse_bind_matrices)
        {
            const uindex _idx = _joint2idx.at(_node);
            mat4 inv_bind = {};
            cgltf_accessor_read_float(_skin.inverse_bind_matrices, _idx, value_ptr(inv_bind), 16);
            return inv_bind;
        }
        else
        {
            // glTF: "When undefined, each matrix is a 4x4 identity matrix."
            // HMM: Why would this even be useful?
            return glm::identity<mat4>();
        }
    };

    const esr::NodeID node_id = _node2node_id.at(_node);

    const uindex joint_idx = skin.joints.size();
    skin.joints.push_back({
        .name        = to_string(_node->name),
        .inv_bind    = inv_bind,
        .parent_idx  = u32(-1), // Will set later when unwinding.
        .child0_idx  = u32(-1), // ''
        .sibling_idx = u32(-1), // ''
        .node_id     = node_id,
    });
    *joint_idx_ptr = joint_idx;
    skin.joint_idxs.emplace(node_id, joint_idx);

    u32 prev_sibling_idx = -1;
    for (const cgltf_node* _child_node : make_span(_node->children, _node->children_count))
    {
        const u32 child_idx = skin.joints.size();
        populate_joints_preorder(skin, _child_node, _joint2preorder_idx,
            _skin, _joint2idx, _node2node_id);

        // Fix-up the relationships.
        auto& child = skin.joints[child_idx]; // Index again, as it could be invalidated in populate().
        auto& joint = skin.joints[joint_idx]; // ''

        child.parent_idx = joint_idx;

        if (joint.child0_idx == u32(-1))
            joint.child0_idx = child_idx;

        if (prev_sibling_idx != u32(-1))
            skin.joints[prev_sibling_idx].sibling_idx = child_idx;

        prev_sibling_idx = child_idx;
    }
}

void populate_node_relationships(
    esr::ExternalScene&                             scene,
    const cgltf_node*                               _node,
    const esr::map<const cgltf_node*, esr::NodeID>& _node2node_id)
{
    assert(_node);

    const esr::NodeID node_id = _node2node_id.at(_node);

    // Pretty much identical to populate_joints_preorder() for this part.
    esr::NodeID prev_sibling_id = esr::null_id;
    for (const cgltf_node* _child_node : make_span(_node->children, _node->children_count))
    {
        const esr::NodeID child_id = _node2node_id.at(_child_node);
        populate_node_relationships(scene, _child_node, _node2node_id);

        auto& child = scene.get<esr::Node>(child_id);
        auto& node  = scene.get<esr::Node>(node_id);

        child.parent_id = node_id;

        if (node.child0_id == esr::null_id)
            node.child0_id = child_id;

        if (prev_sibling_id != esr::null_id)
            scene.get<esr::Node>(prev_sibling_id).sibling_id = child_id;

        prev_sibling_id = child_id;
    }
}

} // namespace

auto to_external_scene(const cgltf_data& gltf, Path _base_dir)
    -> esr::ExternalScene
{
    esr::ExternalScene scene = {
        .base_dir = MOVE(_base_dir),
    };

    /*
    NOTE: Will prefix gltf-specific data with _ to differentiate.
    The terminology for gltf stuff is kept as-is for its identifiers,
    for example:

        | glTF Term  | ESR Term            |
        | ---        | ---                 |
        | _mesh      | [none] (meshbundle) |
        | _primitive | mesh                |

    */

    const auto _scenes     = make_span(gltf.scenes,     gltf.scenes_count    );
    const auto _nodes      = make_span(gltf.nodes,      gltf.nodes_count     );
    const auto _meshes     = make_span(gltf.meshes,     gltf.meshes_count    );
    const auto _images     = make_span(gltf.images,     gltf.images_count    );
    const auto _materials  = make_span(gltf.materials,  gltf.materials_count );
    const auto _lights     = make_span(gltf.lights,     gltf.lights_count    );
    const auto _cameras    = make_span(gltf.cameras,    gltf.cameras_count   );
    const auto _skins      = make_span(gltf.skins,      gltf.skins_count     );
    const auto _animations = make_span(gltf.animations, gltf.animations_count);


    // NOTE: The helper "backward" maps (ex. _node2node_id) are needed so that we
    // could consume all objects by just iterating their arrays first, and then
    // associate them with other objects that reference them (nodes, materials, meshes, etc.).
    // This way, *everything* gets imported even if it is not used in the file directly.

    auto _camera2camera_id = esr::map<const cgltf_camera*, esr::CameraID>();

    for (const cgltf_camera& _camera : _cameras)
    {
        auto [camera_id, camera] = scene.create_as<esr::Camera>({
            // TODO
        });
        _camera2camera_id.emplace(&_camera, camera_id);
    }



    auto _light2light_id = esr::map<const cgltf_light*, esr::LightID>();

    for (const cgltf_light& _light : _lights)
    {
        auto [light_id, light] = scene.create_as<esr::Light>({
            // TODO:
        });
        _light2light_id.emplace(&_light, light_id);
    }



    auto _image2image_id = esr::map<const cgltf_image*, esr::ImageID>();


    for (const cgltf_image& _image : _images)
    {
        auto [image_id, image] = scene.create_as<esr::Image>({
            .path = eval%[&]() -> esr::string {
                if (_image.uri)       return _image.uri;
                if (_image.name)      return _image.name;
                if (_image.mime_type) return _image.mime_type;
                return {};
            },
           .embedded = eval%[&]() -> ElementsView {
                if (_image.buffer_view)
                {
                    return {
                        .bytes         = cgltf_buffer_view_data(_image.buffer_view),
                        .element_count = _image.buffer_view->size,
                        .stride        = 1,
                        .element       = element_u8vec1,
                    };
                }
                return {};
            },
            .width        = {}, // Will fill below.
            .height       = {}, // ''
            .num_channels = {}, // ''
            .is_encoded   = true // NOTE: glTF images are always encoded.
        });

        // NOTE: Here we always query the image info (w, h, num_channels)
        // even if the image is on disk. This info is needed later in the
        // material tear-down pass to tell if certain channles (like alpha)
        // are present or not. glTF sadly does not enforce this info inside
        // the json itself. I'd consider this opaque "channel packing" philosophy
        // to be a defect in the spec.

        const Optional info = eval%[&]{
            if (const auto& data = image.embedded)
            {
                return peek_encoded_image_info({ (const ubyte*)data.bytes, data.element_count });
            }
            else
            {
                const Path filepath = scene.base_dir / image.path;
                return peek_encoded_image_info(filepath.c_str());
            }
        };

        if (not info)
            throw_fmt("Could not get image info for {}.", image.path);

        image.width        = info->resolution.width;
        image.height       = info->resolution.height;
        image.num_channels = info->num_channels;

        _image2image_id.emplace(&_image, image_id);
    } // for (_image)



    // NOTE: We don't parse cgltf_texture into esr::Texture directly, since
    // what glTF defines as a texture is somewhat distant from our representation
    // where we use additional colorspace and swizzle information, and, in practice
    // create a separate esr::Texture per-material per-slot.
    //
    // As such, there isn't a 1-to-1 mapping between cgltf_texture* and esr::TextureID.

    auto _material2material_id = esr::map<const cgltf_material*, esr::MaterialID>();

    for (const cgltf_material& _material : _materials)
    {
        auto [material_id, material] = scene.create_as<esr::Material>({
            .name = to_string(_material.name),
            // Will fill out the rest below.
        });

        // These define the basic source-to-spec swizzles and colorspace conversions
        // for all of the textures. glTF is well-specified with respect to this,
        // maybe *too well specified* since it mandates merged channels for RGB and Alpha,
        // as well as packed metallic and roughness. We don't want that, so there's a
        // texture per slot.
        struct ViewInfo
        {
            SwizzleRGBA swizzle;
            Colorspace  colorspace;
        };

        using enum Swizzle;
        using enum Colorspace;

        const ViewInfo info_color_rgba = { .swizzle={ Red,  Green, Blue, Alpha }, .colorspace=sRGB   };
        const ViewInfo info_color_rgb1 = { .swizzle={ Red,  Green, Blue, One   }, .colorspace=sRGB   };
        const ViewInfo info_metallic   = { .swizzle={ Zero, Zero,  Blue, Zero  }, .colorspace=Linear };
        const ViewInfo info_roughness  = { .swizzle={ Zero, Green, Zero, Zero  }, .colorspace=Linear };
        const ViewInfo info_spec_color = { .swizzle={ Red,  Green, Blue, Zero  }, .colorspace=sRGB   };
        const ViewInfo info_spec_gray  = { .swizzle={ Zero, Zero,  Zero, Alpha }, .colorspace=Linear };
        const ViewInfo info_normal     = { .swizzle={ Red,  Green, Blue, Zero  }, .colorspace=Linear };
        const ViewInfo info_emissive   = { .swizzle={ Red,  Green, Blue, Zero  }, .colorspace=sRGB   };

        auto _create_texture = [&](
            const cgltf_texture& _texture,
            esr::ImageID         image_id,
            const ViewInfo&      info)
                -> auto
        {
            return scene.create_as<esr::Texture>({
                .name         = to_string(_texture.name),
                .image_id     = image_id,
                .swizzle      = info.swizzle,
                .colorspace   = info.colorspace,
                .sampler_info = _texture.sampler ? to_sampler_info(*_texture.sampler) : esr::SamplerInfo{},
            });
        };

        const overloaded create_texture = {
            _create_texture,
            [&](const cgltf_texture& _texture, const ViewInfo& info)
            {
                return _create_texture(_texture, _image2image_id.at(_texture.image), info);
            },
        };

        material.alpha_threshold = _material.alpha_cutoff;
        material.alpha_method = eval%[&]{
            switch (_material.alpha_mode)
            {
                case cgltf_alpha_mode_opaque: return esr::AlphaMethod::None;
                case cgltf_alpha_mode_mask:   return esr::AlphaMethod::Test;
                case cgltf_alpha_mode_blend:  return esr::AlphaMethod::Blend;
                default:                      return esr::AlphaMethod::None;
            }
        };
        material.double_sided = _material.double_sided;

        if (_material.has_pbr_metallic_roughness)
        {
            const cgltf_pbr_metallic_roughness& _mat = _material.pbr_metallic_roughness;

            if (const cgltf_texture* _base_texture = _mat.base_color_texture.texture)
            {
                const esr::ImageID image_id = _image2image_id.at(_base_texture->image);
                const esr::Image&  image    = scene.get<esr::Image>(image_id);

                const bool     has_alpha = (image.num_channels == 4);
                const ViewInfo info      = has_alpha ? info_color_rgba : info_color_rgb1;

                material.color_id = create_texture(*_base_texture, image_id, info).id;
            }
            material.color_factor = vec3(to_vec4(_mat.base_color_factor));
            material.alpha_factor = float(_mat.base_color_factor[3]);

            if (const cgltf_texture* _mr_texture = _mat.metallic_roughness_texture.texture)
            {
                const esr::ImageID image_id = _image2image_id.at(_mr_texture->image);

                // Split into two components.
                // glTF: "Its green channel contains roughness values and its blue channel contains metalness values."
                material.metallic_id  = create_texture(*_mr_texture, image_id, info_metallic).id;
                material.roughness_id = create_texture(*_mr_texture, image_id, info_roughness).id;
            }
            material.roughness_factor = _mat.roughness_factor;
            material.metallic_factor  = _mat.metallic_factor;
        }

        if (_material.has_specular)
        {
            // The old Phong specular will likely masquarade as one of these textures.
            const cgltf_specular& _mat = _material.specular;

            // NOTE: These textures are not specified as merged, but are *very likely*
            // to be merged into a single RGBA texture anyway. Way to go, that's how
            // you do this!
            if (const cgltf_texture* _spec_color_texture = _mat.specular_color_texture.texture)
                material.specular_color_id = create_texture(*_spec_color_texture, info_spec_color).id;

            if (const cgltf_texture* _spec_texture = _mat.specular_texture.texture)
                material.specular_id = create_texture(*_spec_texture, info_spec_gray).id;

            material.specular_color_factor = to_vec3(_mat.specular_color_factor);
            material.specular_factor       = float(_mat.specular_factor);
        }

        if (const cgltf_texture* _normal_texture = _material.normal_texture.texture)
            material.normal_id = create_texture(*_normal_texture, info_normal).id;

        if (const cgltf_texture* _emissive_texture = _material.emissive_texture.texture)
            material.emissive_id = create_texture(*_emissive_texture, info_emissive).id;

        material.emissive_factor   = to_vec3(_material.emissive_factor);
        material.emissive_strength = float(_material.emissive_strength.emissive_strength);

        _material2material_id.emplace(&_material, material_id);
    } // for (_materials)



    // NOTE: glTF "Meshes" are extra annoying because they are not "meshes",
    // but just bundles of *real* renderable meshes (under common definition).
    // We create this temporary entity type just to deal with them in the scene
    // graph, and flatten them later when converting to *our* scene graph format.

    struct MeshBundle
    {
        esr::vector<esr::MeshID> mesh_ids;
    };
    using MeshBundleID = esr::ID;

    auto _mesh2meshbundle_id = esr::map<const cgltf_mesh*, MeshBundleID>();

    for (const cgltf_mesh& _mesh : _meshes)
    {
        auto [meshbundle_id, meshbundle] = scene.create_as<MeshBundle>({
            .mesh_ids = {}, // Fill out per primitive.
        });
        _mesh2meshbundle_id.emplace(&_mesh, meshbundle_id);

        const auto _primitives = make_span(_mesh.primitives, _mesh.primitives_count);
        for (const cgltf_primitive& _primitive : _primitives)
        {
            if (_primitive.type != cgltf_primitive_type_triangles)
                throw GLTFParseError("Primitive types other than triangles are not supported.");

            if (_primitive.has_draco_mesh_compression)
                throw GLTFParseError("Draco mesh compression not supported.");

            Optional<LocalAABB> aabb_opt;
            const esr::MeshAttributes attributes = parse_primitive_attributes(_primitive, &aabb_opt);

            // NOTE: We decide on whether the mesh is skinned based on the presence
            // of respective attributes, even if no skeleton is attached to it.
            const bool is_skinned =
                (attributes.joint_ids and attributes.joint_ws);

            if (is_skinned) validate_attributes_skinned(attributes);
            else            validate_attributes_static (attributes);

            const auto format =
                is_skinned ? VertexFormat::Skinned : VertexFormat::Static;

            const esr::MaterialID material_id =
                _primitive.material ? _material2material_id.at(_primitive.material) : esr::null_id;

            // NOTE: This might actually do an O(N) minmax reduction. Fairly expensive.
            if (not aabb_opt) aabb_opt = compute_aabb(attributes.positions);

            // compute_aabb() can fail if the position attribute is not convertible,
            // but that should never happen given that we validated it before.
            assert(aabb_opt);

            // We do not unpack data, just do validation and emplace views.
            const esr::MeshID mesh_id = scene.create_as<esr::Mesh>({
                .name        = to_string(_mesh.name), // NOTE: Will have duplicate names for multiprimitives.
                .attributes  = attributes,
                .aabb        = aabb_opt.value(),
                .format      = format,
                .material_id = material_id,
                .skin_id     = esr::null_id, // Will be added later, if has a skin to refer to in this file.
            }).id;

            meshbundle.mesh_ids.push_back(mesh_id);
        } // for (_primitives)
    } // for (_meshes)



    // The node population is three-pass. The first deals with transforms
    // and creating a basic _node2node_id lookup table, the second populates
    // the relationships from the respective scenes, and the third populates
    // the entity lists.
    //
    // This is needed because some entities depend on things that themselves
    // depend on the node structure (ex. mesh entities depend on skins, while
    // skins reference nodes).
    //
    // The second and third passes are not merged because we want to scan
    // all of the nodes for entities, even if they (possibly) do not belong
    // to a particular scene.

    auto _node2node_id = esr::map<const cgltf_node*, esr::NodeID>();

    for (const cgltf_node& _node : _nodes) // First node pass: Node creation and transforms.
    {
        // NOTE: On the first pass we do not establish relationships since we are
        // just iterating a flat array and have no way of knowing them.
        auto [node_id, node] = scene.create_as<esr::Node>({
            .name       = to_string(_node.name),
            .entities   = {},           // Fill in the third pass later.
            .transform  = to_transform(_node),
            .parent_id  = esr::null_id, // Fill in the second pass later.
            .child0_id  = esr::null_id, // ''
            .sibling_id = esr::null_id, // ''
        });

        _node2node_id.emplace(&_node, node_id);
    } // for (_node)



    for (const cgltf_scene& _scene : _scenes) // Second node pass: relationships.
    {
        auto [scene_id, scene_] = scene.create_as<esr::Scene>({
            .name          = to_string(_scene.name),
            .root_node_ids = {}, // Fill below.
        });

        // NOTE: All nodes in the _scene.nodes array are root nodes. Spec says.
        // HMM: So there are multiple scenes *and* multiple roots? Eww.
        const auto _roots = make_span(_scene.nodes, _scene.nodes_count);

        for (const cgltf_node* _root : _roots)
        {
            scene_.root_node_ids.push_back(_node2node_id.at(_root));
            populate_node_relationships(scene, _root, _node2node_id);
        }
    } // for (_scene)



    auto _skin2skin_id = esr::map<const cgltf_skin*, esr::SkinID>();

    // We will use this opportunity to tag nodes as "joint" nodes.
    // This technically breaks if multiple skins "share" nodes.
    // I'm not sure why that would ever be the case but the spec
    // does not forbid this (as far as I can tell).
    //
    // We later use this when deciding if an animation channel is
    // controlling a joint or a simple scene-graph node.
    struct JointNode
    {
        esr::SkinID skin_id;
    };

    // NOTE: We do not store ElementViews for inv_bind matrices.
    // This is the only place where we actually read the data in.
    // Or worse yet, compute it if it's not present in the file.

    // NOTE: These datastructures are reused per-skin.
    auto _joint2preorder_idx = esr::map<const cgltf_node*, u32>();
    auto _joint2idx          = esr::map<const cgltf_node*, uindex>();

    for (const cgltf_skin& _skin : _skins)
    {
        _joint2idx.clear();
        _joint2preorder_idx.clear();

        // NOTE: You don't actually need the Nodes constructed here to infer
        // the joint hierarchy. So we can process the skins first.
        auto [skin_id, skin] = scene.create_as<esr::Skin>({
            .name       = to_string(_skin.name),
            .joints     = {}, // Will fill in populate_joints_preorder().
            .joint_idxs = {}, // ''
        });

        // First we just grab a set of joints so that we could quickly
        // test if some node belongs to the current skin.
        const auto _joints = make_span(_skin.joints, _skin.joints_count);
        for (const auto[_idx, _joint] : enumerate(_joints))
        {
            _joint2preorder_idx.emplace(_joint, u32(-1)); // The values will be initialized in populate().
            _joint2idx.emplace(_joint, _idx);
        }

        // Should not be possible, but skip just in case since we rely on this later.
        if (_joints.empty()) continue;

        // Then we find the root of the skeleton by starting at an arbitrary
        // joint and iterating upwards until we fall out of the set.
        const cgltf_node* _root = eval%[&]{
            const cgltf_node* _node = _joints[0];
            while (_joint2preorder_idx.contains(_node->parent))
                _node = _node->parent;
            return _node;
        };

        // Once we have found the root, we can descend in pre-order and populate
        // our joints array in the same order. We also fill the node->idx mapping
        // to establish the relationships in the Joint structures.
        populate_joints_preorder(skin, _root, _joint2preorder_idx,
            _skin, _joint2idx, _node2node_id);

        // Tag all joint nodes as such. We bail if the nodes are "instanced"
        // between multiple skins. I have no idea what that would even mean.
        for (const esr::Joint& joint : skin.joints)
        {
            if (scene.any_of<JointNode>(joint.node_id))
                throw GLTFParseError("Nodes instanced between skins are not supported.");

            scene.emplace<JointNode>(joint.node_id, skin_id);
        }

    } // for (_skin) (hihi)



    for (const cgltf_node& _node : _nodes) // Third node pass: entities.
    {
        auto& node = scene.get<esr::Node>(_node2node_id.at(&_node));

        // Since we want to populate the entity list here, all Meshes, Cameras
        // and Lights must have already been processed before this point.
        //
        // The only exception is the Mesh->Skin association, which is for some
        // reason encoded in the nodes themselves, and can only be recovered here.

        if (_node.camera)
            node.entities.push_back(_camera2camera_id.at(_node.camera));

        if (_node.light)
            node.entities.push_back(_light2light_id.at(_node.light));

        // Here we unpack each glTF "mesh" into a separate mesh
        // entities since we have no use for that multi-mesh representation.
        if (_node.mesh)
        {
            const MeshBundleID meshbundle_id = _mesh2meshbundle_id.at(_node.mesh);
            for (const esr::MeshID mesh_id : scene.get<MeshBundle>(meshbundle_id).mesh_ids)
            {
                node.entities.push_back(mesh_id);
                if (_node.skin)
                {
                    // HMM: What is up with skinned meshes and their skin references?
                    // Why is it referenced at the Node level? Why would a mesh
                    // reference two different skins in different nodes?
                    // How could a multi-mesh "mesh" use the same skin?
                    esr::Mesh& mesh = scene.get<esr::Mesh>(mesh_id);
                    mesh.skin_id = _skin2skin_id.at(_node.skin);
                }
            }
        }
    } // for (_node)



    for (const cgltf_animation& _animation : _animations)
    {
        auto [animation_id, animation] = scene.create_as<esr::Animation>({
            .name             = to_string(_animation.name),
            .node_animations  = {}, // Fill below.
            .skin_animations  = {}, // ''
            .morph_animations = {}, // ''
        });

        // glTF: "Different channels of the same animation MUST NOT have the same targets."
        const auto _channels = make_span(_animation.channels, _animation.channels_count);

        for (const cgltf_animation_channel& _channel : _channels)
        {
            // glTF: "When node isn’t defined, channel SHOULD be ignored."
            // glTF: "When undefined, the animated object MAY be defined by an extension."
            if (not _channel.target_node) continue;

            // glTF: "Within one animation, each target (a combination of a node
            // and a path) MUST NOT be used more than once."

            if (_channel.target_path == cgltf_animation_path_type_weights)
            {
                // TODO: This. Once we care.
                // How do we find the referenced mesh among the "primitives"?
                // Who thought this was a good idea anyway?
            }

            if (_channel.target_path == cgltf_animation_path_type_translation or
                _channel.target_path == cgltf_animation_path_type_rotation or
                _channel.target_path == cgltf_animation_path_type_scale)
            {
                auto assign_trs_channel = [](
                    esr::TRSMotion&                trs_motion,
                    const cgltf_animation_channel& _channel,
                    float&                         duration)
                {
                    // NOTE: Only one MotionChannel is populated per _channel. Makes sense.
                    const esr::MotionChannel channel = {
                        .interpolation = to_motion_interpolation(_channel.sampler->interpolation),
                        .ticks         = to_elements_view(*_channel.sampler->input),
                        .values        = to_elements_view(*_channel.sampler->output),
                    };

                    const auto max_time =
                        copy_convert_one_element<float>(channel.ticks, channel.ticks.element_count - 1);

                    if (max_time > duration)
                        duration = max_time;

                    switch (_channel.target_path)
                    {
                        case cgltf_animation_path_type_translation: trs_motion.translation = channel; break;
                        case cgltf_animation_path_type_rotation:    trs_motion.rotation    = channel; break;
                        case cgltf_animation_path_type_scale:       trs_motion.scaling     = channel; break;
                        default: assert(false); break;
                    }
                };

                const esr::NodeID target_node_id = _node2node_id.at(_channel.target_node);

                // This is stupid, just say upfront that you target a skin joint or node.
                // Why do I have to unravel this mess every time with these "generic node" formats?
                if (const JointNode* joint_node = scene.try_get<JointNode>(target_node_id))
                {
                    const esr::SkinID skin_id = joint_node->skin_id;

                    auto it = animation.skin_animations.find(skin_id);
                    if (it == animation.skin_animations.end())
                    {
                        const auto skin_animation_id = scene.create_as<esr::SkinAnimation>({
                            .name     = to_string(_animation.name),
                            .motions  = {},   // Fill later.
                            .skin_id  = skin_id,
                            .tps      = 1.0f, // glTF is always seconds.
                            .duration = {},   // Fill later, as a max T of all motions.
                        }).id;
                        it = animation.skin_animations.emplace(skin_id, skin_animation_id).first;
                    }
                    const esr::SkinAnimationID skin_animation_id = it->second;
                    esr::SkinAnimation&        skin_animation    = scene.get<esr::SkinAnimation>(skin_animation_id);

                    const esr::Skin& skin      = scene.get<esr::Skin>(skin_animation.skin_id);
                    const u32        joint_idx = skin.joint_idxs.at(target_node_id);

                    // Ah yes, the ".first->second" member. A close relative of the "&*" operator.
                    // The overuse of std::pair in STL style is incredible. Soooo "generic" and "reusable".
                    esr::TRSMotion& trs_motion = skin_animation.motions.try_emplace(joint_idx).first->second;

                    assign_trs_channel(trs_motion, _channel, skin_animation.duration);
                }
                else // Scene-graph node.
                {
                    // NOTE: Only one NodeAnimation is created for this `animation`.
                    if (animation.node_animations.empty())
                    {
                        const auto node_animation_id = scene.create_as<esr::NodeAnimation>({
                            .name     = to_string(_animation.name),
                            .motions  = {}, // Fill later.
                            .tps      = 1.0f,
                            .duration = {},
                        }).id;
                        animation.node_animations.emplace_back(node_animation_id);
                    }
                    const esr::NodeAnimationID node_animation_id = animation.node_animations.back();
                    esr::NodeAnimation&        node_animation    = scene.get<esr::NodeAnimation>(node_animation_id);

                    esr::TRSMotion& trs_motion = node_animation.motions.try_emplace(target_node_id).first->second;

                    assign_trs_channel(trs_motion, _channel, node_animation.duration);
                }
            }
        } // for (_channel)
    } // for (_animation)



    return scene;
}


} // namespace josh::detail::cgltf
