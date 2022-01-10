#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/orthonormalize.hpp>

class Basis3D {
protected:
	glm::vec3 x_;
	glm::vec3 y_;
	glm::vec3 z_;

public: 
	Basis3D(const glm::vec3& x, const glm::vec3& y, const glm::vec3& z) noexcept :
			x_{ x }, y_{ y }, z_{ z } {}

	const glm::vec3& getX() const { return x_; }
	const glm::vec3& getY() const { return y_; }
	const glm::vec3& getZ() const { return z_; }

};

class OrthonormalBasis3D : public Basis3D {
private:
	bool isRightHanded_;

public:
	OrthonormalBasis3D(const glm::vec3& x, const glm::vec3& y, bool isRightHanded = true) : 
		Basis3D(glm::normalize(x),
				glm::orthonormalize(y, x),
				(isRightHanded ? 1.0f : -1.0f) * glm::normalize(glm::cross(x, y))),
			isRightHanded_{ isRightHanded } {}

	void rotate(float angleRad, const glm::vec3& axis) {
		auto rotationMatrix{ glm::rotate(glm::mat4(1.0f), angleRad, axis) };

		// glm implicitly exctracts vec3 from vec4
		x_ = static_cast<glm::vec3>(rotationMatrix * glm::vec4(x_, 1.0f));
		y_ = static_cast<glm::vec3>(rotationMatrix * glm::vec4(y_, 1.0f));
		z_ = static_cast<glm::vec3>(rotationMatrix * glm::vec4(z_, 1.0f));
	}

	static OrthonormalBasis3D invert(const OrthonormalBasis3D& basis) {
		return { -basis.x_, -basis.y_, !basis.isRightHanded_ };
	}

	bool isRightHanded() const { return isRightHanded_; }


};

