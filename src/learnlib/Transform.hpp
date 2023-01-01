#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace learn {


class Transform {
private:
    glm::mat4 model_{ 1.0f };

public:
    Transform() = default;

    explicit(false) Transform(const glm::mat4& model) : model_{ model } {}

    glm::mat4 model() const noexcept {
        return model_;
    }

    glm::mat3 normal_model() const noexcept {
        return glm::transpose(glm::inverse(model_));
    }


    Transform& translate(const glm::vec3& delta) & noexcept {
        model_ = glm::translate(model_, delta);
        return *this;
    }

    Transform translate(const glm::vec3& delta) && noexcept {
        return glm::translate(model_, delta);
    }


    Transform& rotate(float angle_rad, const glm::vec3& axis) & noexcept {
        model_ = glm::rotate(model_, angle_rad, axis);
        return *this;
    }

    Transform rotate(float angle_rad, const glm::vec3& axis) && noexcept {
        return glm::rotate(model_, angle_rad, axis);
    }


    Transform& scale(const glm::vec3& xyz_scaling) & noexcept {
        model_ = glm::scale(model_, xyz_scaling);
        return *this;
    }

    Transform scale(const glm::vec3& xyz_scaling) && noexcept {
        return glm::scale(model_, xyz_scaling);
    }

};


} // namespace learn
