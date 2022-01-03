#pragma once
#include <utility>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"

extern float deltaFrameTime;
extern const OrthonormalBasis3D globalBasis;

class IInput {
public: 
	virtual void processInput() = 0;
};

class InputGlobal : public IInput {
protected:
	GLFWwindow* m_window;

public:
	InputGlobal(GLFWwindow* window) : m_window{ window } {}

	virtual void processInput() override {
		processInputGlobal();
	}

protected:
	void processInputGlobal() {

		if ( glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS ) {
			while ( glfwGetKey(m_window, GLFW_KEY_ESCAPE) != GLFW_RELEASE ) {
				glfwPollEvents();
			}
			glfwSetWindowShouldClose(m_window, true);
		}

		static bool isLineMode{ false };
		if ( glfwGetKey(m_window, GLFW_KEY_H) == GLFW_PRESS ) {
			while ( glfwGetKey(m_window, GLFW_KEY_H) != GLFW_RELEASE ) {
				glfwPollEvents();
			}
			glPolygonMode(GL_FRONT_AND_BACK, isLineMode ? GL_LINE : GL_FILL);
			isLineMode ^= true;
		}

	}


};



class InputFreeCamera : public InputGlobal {
private:
	Camera& m_camera;

public:
	InputFreeCamera(GLFWwindow* window, Camera& camera) : InputGlobal{ window }, m_camera { camera } {
		
		// https://www.glfw.org/faq.html#216---how-do-i-use-c-methods-as-callbacks 
		setThisAsUserPointer(window); // oh. my. fucking. god. how. I. hate. C. APIs.
		
		glfwSetCursorPosCallback(m_window,
			&InputFreeCamera::processInputCameraRotate);
		
		glfwSetScrollCallback(m_window,
			&InputFreeCamera::processInputCameraZoom);
	}

	void setThisAsUserPointer(GLFWwindow* window) {
		glfwSetWindowUserPointer(window, this);
	}

	virtual void processInput() override {
		processInputGlobal();
		processInputCameraMove();
	}

private:
	void processInputCameraMove() {
		
		constexpr float cameraSpeed{ 5.0f };
		float cameraAbsMove{ cameraSpeed * deltaFrameTime };
		if ( glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS )
			m_camera.move(cameraAbsMove * -m_camera.backUV());
		if ( glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS )
			m_camera.move(cameraAbsMove * m_camera.backUV());
		if ( glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS )
			m_camera.move(cameraAbsMove * -m_camera.rightUV());
		if ( glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS )
			m_camera.move(cameraAbsMove * m_camera.rightUV());
		if ( glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS )
			m_camera.move(cameraAbsMove * -m_camera.upUV());
		if ( glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS )
			m_camera.move(cameraAbsMove * m_camera.upUV());

	}


	void static processInputCameraRotate(GLFWwindow* window, double xpos, double ypos) {
		InputFreeCamera* objPtr{ static_cast<InputFreeCamera*>(glfwGetWindowUserPointer(window)) };
		Camera& cam{ objPtr->m_camera };

		static float lastXPos{ 0.0f };
		static float lastYPos{ 0.0f };

		float sensitivity{ 0.1f * cam.getFOV() };

		float xOffset{ static_cast<float>(xpos) - lastXPos };
		xOffset = glm::radians(sensitivity * xOffset);
		
		float yOffset{ static_cast<float>(ypos) - lastYPos };
		yOffset = glm::radians(sensitivity * yOffset);
		
		lastXPos = static_cast<float>(xpos);
		lastYPos = static_cast<float>(ypos);

		cam.rotate(xOffset, -globalBasis.getY());
		cam.rotate(yOffset, -cam.rightUV());

	}

	void static processInputCameraZoom(GLFWwindow* window, double, double yoffset) {
		InputFreeCamera* objPtr{ static_cast<InputFreeCamera*>(glfwGetWindowUserPointer(window)) };
		Camera& cam{ objPtr->m_camera };

		constexpr float sensitivity{ 2.0f };
		cam.setFOV(cam.getFOV() - sensitivity * glm::radians(static_cast<float>(yoffset)));
		if ( cam.getFOV() < glm::radians(1.0f) )
			cam.setFOV(glm::radians(1.0f));
		if ( cam.getFOV() > glm::radians(135.0f) )
			cam.setFOV(glm::radians(135.0f));

	}

};





