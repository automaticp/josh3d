#pragma once
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>


namespace josh {


// Convenience to express the standard basis in local space.
//
// TODO: A separate header for this is overkill.
// But maybe move this somewhere else still.
constexpr glm::vec3 X{ 1.f, 0.f, 0.f };
constexpr glm::vec3 Y{ 0.f, 1.f, 0.f };
constexpr glm::vec3 Z{ 0.f, 0.f, 1.f };



class MTransform;


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
class Transform {
private:
    glm::vec3 position_   { 0.f, 0.f, 0.f };
    glm::quat orientation_{ 1.f, 0.f, 0.f, 0.f };
    glm::vec3 scale_      { 1.f, 1.f, 1.f };

public:
    Transform() = default;

    Transform(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& scale)
        : position_   { position    }
        , orientation_{ orientation }
        , scale_      { scale       }
    {}

    const glm::vec3& position()    const noexcept { return position_;    }
    const glm::quat& orientation() const noexcept { return orientation_; }
    const glm::vec3& scaling()     const noexcept { return scale_;       }

    glm::vec3& position()    noexcept { return position_;    }
    glm::quat& orientation() noexcept { return orientation_; }
    glm::vec3& scaling()     noexcept { return scale_;       }



    Transform& translate(const glm::vec3& delta) noexcept {
        position_ += delta;
        return *this;
    }

    Transform& rotate(const glm::quat& quaternion) noexcept {
        orientation_ *= quaternion;
        return *this;
    }

    Transform& rotate(float angle_rad, const glm::vec3& axis) noexcept {
        orientation_ = glm::rotate(orientation_, angle_rad, axis);
        return *this;
    }

    Transform& scale(const glm::vec3& scale) noexcept {
        scale_ *= scale;
        return *this;
    }


    // Get an euler angle representation of rotation:
    // (X, Y, Z) == (Pitch, Yaw, Roll).
    // Differs from GLM in that the locking axis is Pitch not Yaw.
    glm::vec3 get_euler() const noexcept {
        const auto& q = orientation_;
        const glm::quat q_shfl{ q.w, q.y, q.x, q.z };

        const glm::vec3 euler{
            glm::yaw(q_shfl),   // Pitch
            glm::pitch(q_shfl), // Yaw
            glm::roll(q_shfl)   // Roll
        };
        return euler;
    }

    // Sets the rotation from euler angles:
    // (X, Y, Z) == (Pitch, Yaw, Roll).
    // Works with angles taken from get_euler(),
    // NOT with GLM's eulerAngles().
    void set_euler(const glm::vec3& euler) noexcept {
        const glm::quat p{ glm::vec3{ euler.y, euler.x, euler.z } };
        orientation_ = glm::quat{ p.w, p.y, p.x, p.z };
    }


    // Compute a local MTransform (aka. Model/World matrix) from this Transform.
    MTransform mtransform() const noexcept;

};




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
class MTransform {
private:
    glm::mat4 model_{ 1.0f };

public:
    MTransform() = default;

    explicit(false) MTransform(const glm::mat4& model) : model_{ model } {}

    // Aka. world->local change-of-basis.
    const glm::mat4& model() const noexcept {
        return model_;
    }

    glm::mat3 normal_model() const noexcept {
        return glm::transpose(glm::inverse(model_));
    }


    MTransform& translate(const glm::vec3& delta) noexcept {
        model_ = glm::translate(model_, delta);
        return *this;
    }

    MTransform& rotate(const glm::quat& quaternion) noexcept {
        model_ = model_ * glm::mat4_cast(quaternion);
        return *this;
    }

    MTransform& rotate(float angle_rad, const glm::vec3& axis) noexcept {
        model_ = glm::rotate(model_, angle_rad, axis);
        return *this;
    }

    MTransform& scale(const glm::vec3& xyz_scaling) noexcept {
        model_ = glm::scale(model_, xyz_scaling);
        return *this;
    }


    MTransform operator*(const MTransform& other) const noexcept {
        return { this->model() * other.model() };
    }


    glm::vec3 decompose_position() const noexcept {
        return { model_[3] };
    }

    glm::vec3 decompose_local_scale() const noexcept {
        return glm::sqrt(
            glm::vec3{
                glm::length2(model_[0]),
                glm::length2(model_[1]),
                glm::length2(model_[2])
            }
        );
    }


};




inline MTransform Transform::mtransform() const noexcept {
    return MTransform()
        .translate(position_)
        .rotate(orientation_)
        .scale(scale_);
}



} // namespace josh
