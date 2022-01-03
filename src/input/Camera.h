#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Basis.h"

extern const OrthonormalBasis3D globalBasis;

class Camera {
private:
	glm::vec3 m_pos;
	OrthonormalBasis3D m_localBasis;			// x: right, y: up,   z: back
	float m_fov;

public:
	Camera(const glm::vec3& pos, const glm::vec3& dir, float fov = glm::radians(60.0f)) :
		m_pos{ pos },
		m_localBasis(glm::cross(glm::normalize(dir), globalBasis.getY()),
			glm::orthonormalize(globalBasis.getY(), glm::normalize(dir)), true),
		m_fov{ fov } {}


	glm::mat4 getViewMat() const { return glm::lookAt(m_pos, m_pos - m_localBasis.getZ(), m_localBasis.getY()); }

	float getFOV() const { return m_fov; }
	void setFOV(float fov) { m_fov = fov; }

	void rotate(float angleRad, const glm::vec3& axis) { 
		m_localBasis.rotate(angleRad, axis);
	}
	
	void move(const glm::vec3& deltaVector) {
		m_pos += deltaVector;
	}

	float getPitch() const { 
		return glm::sign(glm::dot(globalBasis.getY(), m_localBasis.getY())) * glm::asin(glm::length(glm::cross(globalBasis.getY(), m_localBasis.getY())));
	}
	const glm::vec3& getPos() { return m_pos; }
	
	// FIXME: make return types into const ref 
	// so that front, left and down are not local to the getters
	//glm::vec3 frontUV() const { return -m_localBasis.getZ(); }
	const glm::vec3& backUV() const { return m_localBasis.getZ(); }
	//glm::vec3 leftUV() const { return -m_localBasis.getX(); }
	const glm::vec3& rightUV() const { return m_localBasis.getX(); }
	//glm::vec3 downUV() const { return -m_localBasis.getY(); }
	const glm::vec3& upUV() const { return m_localBasis.getY(); }

};

