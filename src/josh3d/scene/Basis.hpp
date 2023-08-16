#pragma once
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/orthonormalize.hpp>



namespace josh {



class Basis3D {
protected:
    glm::vec3 x_;
    glm::vec3 y_;
    glm::vec3 z_;

public:
    constexpr Basis3D(const glm::vec3& x, const glm::vec3& y, const glm::vec3& z) noexcept :
        x_{ x }, y_{ y }, z_{ z }
    {}

    constexpr const glm::vec3& x() const noexcept { return x_; }
    constexpr const glm::vec3& y() const noexcept { return y_; }
    constexpr const glm::vec3& z() const noexcept { return z_; }

};


/*
Global Reference

       up
       |
       |
       |________ right
      /
     /
    /
   back


Right-Handed Basis
[X, Y] = Z

       Y
       |
       |
       |________ X
      /
     /
    /
   Z

    Z   Y
    |  /
    | /
    |/________ X


Left-Handed Basis
[X, Y] = -Z

    Y   Z
    |  /
    | /
    |/________ X


       Z
       |
       |
       |________ X
      /
     /
    /
   Y


*/



// FIXME: This inheritance is meh.
class OrthonormalBasis3D : public Basis3D {
private:
    // FIXME: This probably should be a separate type.
    bool is_right_handed_;

public:
    OrthonormalBasis3D(const glm::vec3& x, const glm::vec3& y, bool is_right_handed = true) noexcept :
        Basis3D{
            glm::normalize(x),
            glm::orthonormalize(y, x),
            (is_right_handed ? 1.0f : -1.0f) * glm::normalize(glm::cross(x, y))
        },
        is_right_handed_{ is_right_handed }
    {}

    void rotate(float angle_rad, const glm::vec3& axis) noexcept {
        x_ = glm::rotate(x_, angle_rad, axis);
        y_ = glm::rotate(y_, angle_rad, axis);
        z_ = glm::rotate(z_, angle_rad, axis);
    }

    void rotate(const glm::quat& quat) noexcept {
        x_ = glm::rotate(quat, x_);
        y_ = glm::rotate(quat, y_);
        z_ = glm::rotate(quat, z_);
    }

    OrthonormalBasis3D inverted() const noexcept {
        return { -x_, -y_, !is_right_handed_ };
    }

    bool is_right_handed() const noexcept { return is_right_handed_; }

};


// inline const OrthonormalBasis3D globals::basis{ glm::vec3(1.0f, 0.0, 0.0), glm::vec3(0.0f, 1.0f, 0.0f), true };


} // namespace josh
