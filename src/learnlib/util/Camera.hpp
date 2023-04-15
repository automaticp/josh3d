#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Basis.hpp"
#include "GlobalsUtil.hpp"


namespace learn {


class Camera {
private:
    glm::vec3 pos_;

    // X: right, Y: up, Z: back
    OrthonormalBasis3D local_basis_;

    float fov_;

public:
    Camera(const glm::vec3& pos, const glm::vec3& dir, float fov = glm::radians(60.0f)) :
            pos_{ pos },
            local_basis_{
                glm::cross(glm::normalize(dir), globals::basis.y()),
                glm::orthonormalize(globals::basis.y(), glm::normalize(dir)),
                true
            },
            fov_{ fov }
        {}

    Camera() : Camera({ 0.f, 0.f, 0.f, }, { 0.f, 0.f, -1.f }) {}

    glm::mat4 view_mat() const noexcept { return glm::lookAt(pos_, pos_ - local_basis_.z(), local_basis_.y()); }

    glm::mat4 perspective_projection_mat(float aspect_ratio, float z_near = 0.1f, float z_far = 100.f) const noexcept {
        return glm::perspective(fov_, aspect_ratio, z_near, z_far);
    }

    float get_fov() const noexcept { return fov_; }
    void set_fov(float fov) noexcept { fov_ = fov; }

    void rotate(float angle_rad, const glm::vec3& axis) noexcept {
        local_basis_.rotate(angle_rad, axis);
    }

    void move(const glm::vec3& delta_vector) noexcept {
        pos_ += delta_vector;
    }

    float get_pitch() const noexcept {
        return glm::sign(glm::dot(globals::basis.y(), local_basis_.y())) * glm::asin(glm::length(glm::cross(globals::basis.y(), local_basis_.y())));
    }

    const glm::vec3& get_pos() const noexcept { return pos_; }

    const glm::vec3& back_uv() const noexcept { return local_basis_.z(); }
    const glm::vec3& right_uv() const noexcept { return local_basis_.x(); }
    const glm::vec3& up_uv() const noexcept { return local_basis_.y(); }

};


} // namespace learn
