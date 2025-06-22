#pragma once
#include "Math.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext.hpp>


namespace josh {


struct MTransform;

/*
Transform expressed as postion, orientation and scale.

Can be used when the transforms have to be changed frequently
and also queried at the same time.

Transformations are order-independent.

Should be the default choice.

Caveat is that it does not represent skew, and so it is not fully
equivalent to a 4x4 homogeneous transformation matrix. Scaling
only applies along the local basis.
*/
struct Transform
{
    // Wow, such incredible encapsulation...
    // I seemed to just *love* typing this out.

    Transform() = default;

    Transform(const vec3& position, const quat& orientation, const vec3& scale)
        : _position   { position    }
        , _orientation{ orientation }
        , _scale      { scale       }
    {}

    auto position()          noexcept ->       vec3& { return _position;    }
    auto orientation()       noexcept ->       quat& { return _orientation; }
    auto scaling()           noexcept ->       vec3& { return _scale;       }
    auto position()    const noexcept -> const vec3& { return _position;    }
    auto orientation() const noexcept -> const quat& { return _orientation; }
    auto scaling()     const noexcept -> const vec3& { return _scale;       }

    auto translate(const vec3& delta) noexcept
        -> Transform&
    {
        _position += delta;
        return *this;
    }

    auto rotate(const quat& quaternion) noexcept
        -> Transform&
    {
        _orientation *= quaternion;
        return *this;
    }

    auto rotate(float angle_rad, const vec3& axis) noexcept
        -> Transform&
    {
        _orientation = glm::rotate(_orientation, angle_rad, axis);
        return *this;
    }

    auto scale(const glm::vec3& scale) noexcept
        -> Transform&
    {
        _scale *= scale;
        return *this;
    }

    // Get an euler angle representation of rotation:
    // (X, Y, Z) == (Pitch, Yaw, Roll).
    // Differs from GLM in that the locking axis is Pitch not Yaw.
    //
    // TODO: This should just be replaced by a corresponding free function.
    auto get_euler() const noexcept -> vec3;

    // Sets the rotation from euler angles:
    // (X, Y, Z) == (Pitch, Yaw, Roll).
    // Works with angles taken from get_euler(),
    // NOT with GLM's eulerAngles().
    void set_euler(const vec3& euler) noexcept;

    // Compute a local MTransform (aka. Model/World matrix) from this Transform.
    auto mtransform() const noexcept -> MTransform;

    // NOTE: Currently keeping this semi-private because a tonn of places
    // depend on the position()/orientation()/scaling() functions.
    vec3 _position    = { 0.f, 0.f, 0.f };
    quat _orientation = { 0.f, 0.f, 0.f, 1.f };
    vec3 _scale       = { 1.f, 1.f, 1.f };
};

/*
Get the Euler angle representation of rotation:
    `(X, Y, Z) == (Pitch, Yaw, Roll)`

Differs from GLM in that the locking axis is Pitch not Yaw.

NOTE: These are technically Tait-Brian angles with mixed
local and global axes. Hence all the gimbal lock fun.
*/
inline auto quat_to_euler(const quat& q) noexcept
    -> vec3
{
    const auto q_shfl = quat::wxyz(q.w, q.y, q.x, q.z);
    return {
        glm::yaw  (q_shfl), // Pitch
        glm::pitch(q_shfl), // Yaw
        glm::roll (q_shfl), // Roll
    };
}

/*
Creates a rotation quaternion from Euler angles:
    `(X, Y, Z) == (Pitch, Yaw, Roll)`

Works with angles taken from quat_to_euler(),
NOT with GLM's eulerAngles().
*/
inline auto euler_to_quat(const vec3& euler) noexcept
    -> quat
{
    // This quaternion constructor is insane and will
    // just interpret a vec3 argument as euler angles
    // WTF GLM?
    const quat p = { vec3{ euler.y, euler.x, euler.z } };
    return quat::wxyz(p.w, p.y, p.x, p.z);
}

inline auto Transform::get_euler() const noexcept
    -> vec3
{
    return quat_to_euler(_orientation);
}

inline void Transform::set_euler(const vec3& euler) noexcept
{
    _orientation = euler_to_quat(euler);
}


/*
Transform expressed as a model matrix.

Can be used when the tranform has to be set and possibly
modified but never queried for position, rotation or scale.

Transformations are order-dependent,
translate->rotate->scale for most sane results.

Read matrix multiplication left-to-right: T * R * S.
Primarily used for rendering and parent-child transformation chaining,
use plain Transform in other cases.
*/
struct MTransform
{
    MTransform() : _mat{ glm::identity<mat4>() } {}
    explicit(false) MTransform(const mat4& model) : _mat{ model } {}

    // Aka. world->local change-of-basis.
    auto model() const noexcept -> const mat4& { return _mat; }
    auto normal_model() const noexcept -> mat3 { return transpose(inverse(_mat)); }

    auto translate(const vec3& delta) noexcept
        -> MTransform&
    {
        _mat = glm::translate(_mat, delta);
        return *this;
    }

    auto rotate(const quat& quaternion) noexcept
        -> MTransform&
    {
        _mat = _mat * glm::mat4_cast(quaternion);
        return *this;
    }

    auto rotate(float angle_rad, const vec3& axis) noexcept
        -> MTransform&
    {
        _mat = glm::rotate(_mat, angle_rad, axis);
        return *this;
    }

    auto scale(const vec3& xyz_scaling) noexcept
        -> MTransform&
    {
        _mat = glm::scale(_mat, xyz_scaling);
        return *this;
    }

    auto operator*(const MTransform& other) const noexcept
        -> MTransform
    {
        return { this->model() * other.model() };
    }

    auto decompose_position() const noexcept
        -> vec3
    {
        return { _mat[3] };
    }

    auto decompose_local_scale() const noexcept
        -> vec3
    {
        return glm::sqrt(
            vec3{
                glm::length2(_mat[0]),
                glm::length2(_mat[1]),
                glm::length2(_mat[2])
            }
        );
    }

    operator const mat4&() const noexcept { return _mat; }

    // Same story as with Transform.
    mat4 _mat{ 1.0f };
};


inline auto decompose_translation(const mat4& mat) noexcept
    -> vec3
{
    return { mat[3] };
}

inline auto decompose_local_scale(const mat4& mat) noexcept
    -> vec3
{
    const vec3 l2s_of_bases = {
        glm::length2(mat[0]),
        glm::length2(mat[1]),
        glm::length2(mat[2])
    };
    return glm::sqrt(l2s_of_bases);
}

/*
NOTE: Skew/Shear is not preserved.
HMM: Is returning Transform a reasonable idea?
*/
inline auto decompose_trs(const mat4& mat) noexcept
    -> Transform
{
    vec3 s;
    quat r;
    vec3 t;
    vec3 _skew;
    vec4 _perspective;
    // TODO: We don't care about skew and perspective.
    // Is there a simpler implementation?
    glm::decompose(mat, s, r, t, _skew, _perspective);
    return { t, r, s };
}

inline auto decompose_rotation(const mat4& mat) noexcept
    -> quat
{
    // TODO: Surely there's a simpler way.
    return decompose_trs(mat).orientation();
}

inline auto Transform::mtransform() const noexcept
    -> MTransform
{
    return MTransform()
        .translate(_position)
        .rotate(_orientation)
        .scale(_scale);
}


} // namespace josh
