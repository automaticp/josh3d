#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/orthonormalize.hpp>

class Basis3D {
protected:
	glm::vec3 m_x;
	glm::vec3 m_y;
	glm::vec3 m_z;

public: 
	Basis3D(const glm::vec3& x, const glm::vec3& y, const glm::vec3& z) noexcept :
		m_x{ x }, m_y{ y }, m_z{ z } {}

	const glm::vec3& getX() const { return m_x; }
	const glm::vec3& getY() const { return m_y; }
	const glm::vec3& getZ() const { return m_z; }

};

class OrthonormalBasis3D : public Basis3D {
private:
	bool m_isRightHanded;

public:
	OrthonormalBasis3D(const glm::vec3& x, const glm::vec3& y, bool isRightHanded = true) : 
		Basis3D(glm::normalize(x),
				glm::orthonormalize(y, x),
				(isRightHanded ? 1.0f : -1.0f) * glm::normalize(glm::cross(x, y))),
		m_isRightHanded{isRightHanded} {}

	void rotate(float angleRad, const glm::vec3& axis) {
		auto rotationMatrix{ glm::rotate(glm::mat4(1.0f), angleRad, axis) };

		// glm implicitly exctracts vec3 from vec4
		m_x = static_cast<glm::vec3>(rotationMatrix * glm::vec4(m_x, 1.0f));
		m_y = static_cast<glm::vec3>(rotationMatrix * glm::vec4(m_y, 1.0f));
		m_z = static_cast<glm::vec3>(rotationMatrix * glm::vec4(m_z, 1.0f));
	}

	static OrthonormalBasis3D invert(const OrthonormalBasis3D& basis) {
		return { -basis.m_x, -basis.m_y, !basis.m_isRightHanded };
	}

	bool isRightHanded() const { return m_isRightHanded; }


};

