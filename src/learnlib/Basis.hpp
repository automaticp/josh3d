#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/orthonormalize.hpp>



namespace learn {



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




class OrthonormalBasis3D : public Basis3D {
public:
    const bool right_handed;

    OrthonormalBasis3D(const glm::vec3& x, const glm::vec3& y, bool is_right_handed = true) noexcept :
        Basis3D{
            glm::normalize(x),
            glm::orthonormalize(y, x),
            (is_right_handed ? 1.0f : -1.0f) * glm::normalize(glm::cross(x, y))
        },
        right_handed{ is_right_handed }
    {}

    void rotate(float angle_rad, const glm::vec3& axis) noexcept {
        auto rotation_matrix{ glm::rotate(glm::mat4(1.0f), angle_rad, axis) };

        // glm implicitly converts vec4 to vec3
        x_ = rotation_matrix * glm::vec4(x_, 1.0f);
        y_ = rotation_matrix * glm::vec4(y_, 1.0f);
        z_ = rotation_matrix * glm::vec4(z_, 1.0f);
    }

    static OrthonormalBasis3D invert(const OrthonormalBasis3D& basis) noexcept {
        return { -basis.x_, -basis.y_, !basis.right_handed };
    }

};


// inline const OrthonormalBasis3D globals::basis{ glm::vec3(1.0f, 0.0, 0.0), glm::vec3(0.0f, 1.0f, 0.0f), true };


} // namespace learn
