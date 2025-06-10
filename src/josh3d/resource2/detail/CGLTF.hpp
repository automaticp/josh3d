#pragma once
#include "AABB.hpp"
#include "Common.hpp"
#include "Elements.hpp"
#include "EnumUtils.hpp"
#include "ExternalScene.hpp"
#include "RuntimeError.hpp"
#include "Transform.hpp"
#include <cgltf.h>


// NOTE: Placed in global namespace for ADL.
JOSH3D_DEFINE_ENUM_EXTRAS(
    cgltf_result,
	cgltf_result_success,
	cgltf_result_data_too_short,
	cgltf_result_unknown_format,
	cgltf_result_invalid_json,
	cgltf_result_invalid_gltf,
	cgltf_result_invalid_options,
	cgltf_result_file_not_found,
	cgltf_result_io_error,
	cgltf_result_out_of_memory,
	cgltf_result_legacy_gltf
);

JOSH3D_DEFINE_ENUM_EXTRAS(
    cgltf_type,
    cgltf_type_invalid,
	cgltf_type_scalar,
	cgltf_type_vec2,
	cgltf_type_vec3,
	cgltf_type_vec4,
	cgltf_type_mat2,
	cgltf_type_mat3,
	cgltf_type_mat4
);

namespace josh::detail::cgltf {

struct GLTFParseError : RuntimeError
{
    using RuntimeError::RuntimeError;
};

struct DataDeleter
{
    void operator()(cgltf_data* p) const noexcept { cgltf_free(p); }
};

using unique_data_ptr = std::unique_ptr<cgltf_data, DataDeleter>;

inline auto to_vec3(const float (&v)[3]) noexcept
    -> vec3
{
    return { v[0], v[1], v[2] };
}

inline auto to_vec4(const float (&v)[4]) noexcept
    -> vec4
{
    return { v[0], v[1], v[2], v[3] };
}

inline auto to_quat(const float (&q)[4]) noexcept
    -> quat
{
    // glTF: "rotation is a unit quaternion value, XYZW, in the local coordinate system, where W is the scalar."
    return quat::wxyz(q[3], q[0], q[1], q[2]);
}

inline auto to_mat4(const float (&m)[16]) noexcept
    -> mat4
{
    mat4 result;
    std::memcpy(&result, m, sizeof(mat4)); // Yeah, no, I'm not typing 16 accesses out.
    return result;
}

inline auto to_string(const char* cstr_or_null)
    -> esr::string
{
    // This is such a vile footgun that only the most dedicated
    // "zero-overhead" fanatics could think it is a good idea.
    if (not cstr_or_null) return {};
    return { cstr_or_null };
}

auto to_transform(const cgltf_node& node) noexcept
    -> Transform;

auto to_local_aabb(const cgltf_accessor& accessor) noexcept
    -> Optional<LocalAABB>;

auto to_sampler_info(const cgltf_sampler& sampler) noexcept
    -> esr::SamplerInfo;

auto to_motion_interpolation(cgltf_interpolation_type interp) noexcept
    -> esr::MotionInterpolation;

/*
Will return an Element representation of type and layout, or nullopt
if the conversion cannot be made (ex. when layout is matN or any of
the enums have invalid values).
*/
auto to_element(cgltf_component_type _component_type, cgltf_type _layout, bool normalized = false)
    -> Optional<Element>;

/*
Will return return a view of the accessed elements or an
empty view if no conversion could be made.
*/
auto to_elements_view(const cgltf_accessor& accessor)
    -> ElementsView;

/*
Returns a view for each accessor, of null view if none for each type.
Optionally sets AABB, if the min/max is present. If `aabb` is passed but
the resulting optional is nullopt after this call, then there was no
min/max data in the position accessor or the conversion could not be made.
*/
auto parse_primitive_attributes(
    const cgltf_primitive& _primitive,
    Optional<LocalAABB>*   aabb = nullptr)
        -> esr::MeshAttributes;

/*
Parse `gltf` and create an `ExternalSceneRepr` over the source gltf data.

The `gltf` should be kept alive as long as the returned ESR
as it will keep views to the source gltf buffers.

NOTE: This is relatively expensive and will allocate some memory.
PRE: `gltf` has all buffer data loaded with `cgltf_load_buffers()`.
*/
[[nodiscard]]
auto to_external_scene(const cgltf_data& gltf, Path base_dir)
    -> esr::ExternalScene;


} // namespace josh::detail::cgltf
