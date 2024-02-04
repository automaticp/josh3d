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

class MTransform;

/*
Transform expressed as postion, rotation and scale.

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
    glm::vec3 position_{ 0.f };
    glm::quat rotation_{ 1.f, 0.f, 0.f, 0.f };
    glm::vec3 scale_{ 1.f };

public:
    Transform() = default;

    Transform(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
        : position_{ position }
        , rotation_{ rotation }
        , scale_{ scale }
    {}

    const glm::vec3& position() const noexcept { return position_; }
    const glm::quat& rotation() const noexcept { return rotation_; }
    const glm::vec3& scaling() const noexcept { return scale_; }

    glm::vec3& position() noexcept { return position_; }
    glm::quat& rotation() noexcept { return rotation_; }
    glm::vec3& scaling() noexcept { return scale_; }



    Transform& translate(const glm::vec3& delta) noexcept {
        position_ += delta;
        return *this;
    }

    Transform& rotate(const glm::quat& quaternion) noexcept {
        rotation_ *= quaternion;
        return *this;
    }

    Transform& rotate(float angle_rad, const glm::vec3& axis) noexcept {
        rotation_ = glm::rotate(rotation_, angle_rad, axis);
        return *this;
    }

    Transform& scale(const glm::vec3& scale) noexcept {
        scale_ *= scale;
        return *this;
    }


    [[deprecated("The Transform does not describe the Skew transformation, therefore, \
        it cannot properly propagate Scaling after Rotation. This operator is not equivalent \
        to a model matrix multiplication. Do not use this.")]]
    Transform operator*(const Transform& other) const noexcept {
        return {
            this->position_ + this->scale_ * (this->rotation_ * other.position_),
            this->rotation_ * other.rotation_,
            this->scale_    * other.scale_
        };
    }


    // Get an euler angle representation of rotation:
    // (X, Y, Z) == (Pitch, Yaw, Roll).
    // Differs from GLM in that the locking axis is Pitch not Yaw.
    glm::vec3 get_euler() const noexcept {
        const auto& q = rotation_;
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
        rotation_ = glm::quat{ p.w, p.y, p.x, p.z };
    }


    MTransform mtransform() const noexcept;

    operator MTransform() const noexcept;


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

    const glm::mat4& model() const noexcept {
        return model_;
    }

    glm::mat3 normal_model() const noexcept {
        return glm::transpose(glm::inverse(model_));
    }


    // FIXME: Why do these && overloads even exist, eh?

    MTransform& translate(const glm::vec3& delta) & noexcept {
        model_ = glm::translate(model_, delta);
        return *this;
    }

    MTransform translate(const glm::vec3& delta) && noexcept {
        return glm::translate(model_, delta);
    }


    MTransform& rotate(const glm::quat& quaternion) & noexcept {
        model_ = model_ * glm::mat4_cast(quaternion);
        return *this;
    }

    MTransform rotate(const glm::quat& quaternion) && noexcept {
        return model_ * glm::mat4_cast(quaternion);
    }

    MTransform& rotate(float angle_rad, const glm::vec3& axis) & noexcept {
        model_ = glm::rotate(model_, angle_rad, axis);
        return *this;
    }

    MTransform rotate(float angle_rad, const glm::vec3& axis) && noexcept {
        return glm::rotate(model_, angle_rad, axis);
    }


    MTransform& scale(const glm::vec3& xyz_scaling) & noexcept {
        model_ = glm::scale(model_, xyz_scaling);
        return *this;
    }

    MTransform scale(const glm::vec3& xyz_scaling) && noexcept {
        return glm::scale(model_, xyz_scaling);
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
    return *this;
}




inline Transform::operator MTransform() const noexcept {
    return MTransform()
        .translate(position_)
        .rotate(rotation_)
        .scale(scale_);
}




} // namespace josh
