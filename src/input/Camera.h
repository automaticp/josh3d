#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Basis.h"

extern const OrthonormalBasis3D globalBasis;

class Camera {
private:
	glm::vec3 pos_;
	OrthonormalBasis3D localBasis_;			// x: right, y: up,   z: back
	float fov_;

public:
	Camera(const glm::vec3& pos, const glm::vec3& dir, float fov = glm::radians(60.0f)) :
			pos_{ pos },
			localBasis_(glm::cross(glm::normalize(dir), globalBasis.getY()),
			            glm::orthonormalize(globalBasis.getY(), glm::normalize(dir)), true),
			fov_{ fov } {}


	glm::mat4 getViewMat() const { return glm::lookAt(pos_, pos_ - localBasis_.getZ(), localBasis_.getY()); }

	float getFOV() const { return fov_; }
	void setFOV(float fov) { fov_ = fov; }

	void rotate(float angleRad, const glm::vec3& axis) { 
		localBasis_.rotate(angleRad, axis);
	}
	
	void move(const glm::vec3& deltaVector) {
		pos_ += deltaVector;
	}

	float getPitch() const { 
		return glm::sign(glm::dot(globalBasis.getY(), localBasis_.getY())) * glm::asin(glm::length(glm::cross(globalBasis.getY(), localBasis_.getY())));
	}
	const glm::vec3& getPos() { return pos_; }
	
	// FIXME: make return types into const ref 
	// so that front, left and down are not local to the getters
	//glm::vec3 frontUV() const { return -localBasis_.getZ(); }
	const glm::vec3& backUV() const { return localBasis_.getZ(); }
	//glm::vec3 leftUV() const { return -localBasis_.getX(); }
	const glm::vec3& rightUV() const { return localBasis_.getX(); }
	//glm::vec3 downUV() const { return -localBasis_.getY(); }
	const glm::vec3& upUV() const { return localBasis_.getY(); }

};

