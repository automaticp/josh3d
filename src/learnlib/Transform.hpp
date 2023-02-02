#pragma once
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include <glm/ext.hpp>


namespace learn {


// Transform expressed as a model matrix.
//
// Can be used when the tranform has to be set and
// possibly modified but never queried for position,
// rotation or scale.
//
// Transformations are order-dependent, scale->rotate->translate
// for most sane results.
//
// Primarily used for rendering, use plain Transform in other cases.
class MTransform {
private:
    glm::mat4 model_{ 1.0f };

public:
    MTransform() = default;

    explicit(false) MTransform(const glm::mat4& model) : model_{ model } {}

    glm::mat4 model() const noexcept {
        return model_;
    }

    glm::mat3 normal_model() const noexcept {
        return glm::transpose(glm::inverse(model_));
    }


    // Why do these && overloads even exist, eh?

    MTransform& translate(const glm::vec3& delta) & noexcept {
        model_ = glm::translate(model_, delta);
        return *this;
    }

    MTransform translate(const glm::vec3& delta) && noexcept {
        return glm::translate(model_, delta);
    }


    MTransform& rotate(const glm::quat& quaternion) & noexcept {
        model_ = glm::mat4_cast(quaternion) * model_;
        return *this;
    }

    MTransform rotate(const glm::quat& quaternion) && noexcept {
        return glm::mat4_cast(quaternion) * model_;
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

};



// Transform expressed as postion, rotation and scale.
//
// Can be used when the transforms have to be changed frequently
// and also queried at the same time.
//
// Transformations are order-independent.
//
// Should be the default choice.
class Transform {
private:
    glm::vec3 position_{ 0.f };
    glm::quat rotation_{ 0.f, 0.f, 0.f, 0.f };
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

    MTransform mtransform() const noexcept {
        return *this;
    }

    operator MTransform() const noexcept {
        return MTransform()
            .scale(scale_)
            .rotate(rotation_)
            .translate(position_);
    }

};









} // namespace learn
