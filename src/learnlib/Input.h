#pragma once
#include <functional>
#include <glbinding/gl/gl.h>
#include <glfwpp/glfwpp.h>
#include <glm/glm.hpp>
#include "Camera.h"

using namespace gl;

extern float deltaFrameTime;
extern const OrthonormalBasis3D globalBasis;

class IInput {
public:
	virtual void processInput() = 0;
	virtual ~IInput() = default;
};

class InputGlobal : public IInput {
protected:
	glfw::Window& window_;

public:
	explicit InputGlobal(glfw::Window& window) : window_{ window } {
		using namespace std::placeholders;
		window_.keyEvent.setCallback(std::bind(&InputGlobal::callbackKeys, this, _1, _2, _3, _4, _5));
	}

	virtual void processInput() override {}

protected:
	struct KeysCallbackArgs {
		glfw::Window& window; glfw::KeyCode key; int scancode;
		glfw::KeyState state; glfw::ModifierKeyBit mods;
	};

	virtual void respondInputKeys(KeysCallbackArgs& args) {
		respondInputCloseWindow(args);
		respondInputShowLines(args);
	}

	void callbackKeys(glfw::Window& window, glfw::KeyCode key, int scancode,
		glfw::KeyState state, glfw::ModifierKeyBit mods) {
		KeysCallbackArgs args{ window, key, scancode, state, mods };
		respondInputKeys(args);
	}

	void respondInputCloseWindow(KeysCallbackArgs& args) {
		using namespace glfw;
		if ( args.key == KeyCode::Escape && args.state == KeyState::Release ) {
			args.window.setShouldClose(true);
		}
	}

	void respondInputShowLines(KeysCallbackArgs& args) {
		using namespace glfw;
		static bool isLineMode{ false }; // FIXME: Is it bad that this is shared between all instances?
		if ( args.key == KeyCode::H && args.state == KeyState::Release ) {
			glPolygonMode(GL_FRONT_AND_BACK, isLineMode ? GL_FILL : GL_LINE);
			isLineMode ^= true;
		}
	}

};



class InputFreeCamera : public InputGlobal {
private:
	Camera& camera_;

public:
	InputFreeCamera(glfw::Window& window, Camera& camera) : InputGlobal{ window }, camera_{ camera } {
		using namespace std::placeholders;
		window_.cursorPosEvent.setCallback(std::bind(&InputFreeCamera::callbackCameraRotate, this, _1, _2, _3));
		window_.scrollEvent.setCallback(std::bind(&InputFreeCamera::callbackCameraZoom, this, _1, _2, _3));
	}


	virtual void processInput() override {
		processInputMove();
	}

protected:
	virtual void respondInputKeys(KeysCallbackArgs& args) override {
		respondInputCloseWindow(args);
		respondInputShowLines(args);
		respondInputCameraMove(args);
	}

	struct MoveState {
		bool up		{ false };
		bool down	{ false };
		bool right	{ false };
		bool left	{ false };
		bool back	{ false };
		bool forward{ false };
	};
	MoveState moveState_{};

	void processInputMove() {
		constexpr float cameraSpeed{ 5.0f };
		float absMove{ cameraSpeed * deltaFrameTime };
		glm::vec3 sumMove{ 0.0f, 0.0f, 0.0f };

		if (moveState_.up) 		sumMove += camera_.upUV();
		if (moveState_.down)	sumMove -= camera_.upUV();
		if (moveState_.right)	sumMove += camera_.rightUV();
		if (moveState_.left)	sumMove -= camera_.rightUV();
		if (moveState_.back)	sumMove += camera_.backUV();
		if (moveState_.forward)	sumMove -= camera_.backUV();
		// FIXME
		if ( sumMove != glm::vec3{ 0.0f, 0.0f, 0.0f } ) {
			camera_.move(absMove * glm::normalize(sumMove));
		}
	}

	void respondInputCameraMove(KeysCallbackArgs& args) {
		using namespace glfw;

		if ( args.state == KeyState::Press || args.state == KeyState::Release ) {
			bool state{ static_cast<bool>(args.state) };
			if 		( args.key == KeyCode::W ) 			moveState_.forward	= state;
			else if ( args.key == KeyCode::S ) 			moveState_.back 	= state;
			else if ( args.key == KeyCode::A ) 			moveState_.left 	= state;
			else if ( args.key == KeyCode::D ) 			moveState_.right 	= state;
			else if ( args.key == KeyCode::LeftShift ) 	moveState_.down		= state;
			else if ( args.key == KeyCode::Space ) 	   	moveState_.up 		= state;

		}
	}

	void callbackCameraRotate(glfw::Window& window, double xpos, double ypos) {

		static float lastXPos{ 0.0f };
		static float lastYPos{ 0.0f };

		float sensitivity{ 0.1f * camera_.getFOV() };

		float xOffset{ static_cast<float>(xpos) - lastXPos };
		xOffset = glm::radians(sensitivity * xOffset);

		float yOffset{ static_cast<float>(ypos) - lastYPos };
		yOffset = glm::radians(sensitivity * yOffset);

		lastXPos = static_cast<float>(xpos);
		lastYPos = static_cast<float>(ypos);

		camera_.rotate(xOffset, -globalBasis.getY());
		camera_.rotate(yOffset, -camera_.rightUV());

	}

	void callbackCameraZoom(glfw::Window& window, double, double yoffset) {

		constexpr float sensitivity{ 2.0f };

		camera_.setFOV(camera_.getFOV() - sensitivity * glm::radians(static_cast<float>(yoffset)));
		if ( camera_.getFOV() < glm::radians(1.0f) )
			camera_.setFOV(glm::radians(1.0f));
		if ( camera_.getFOV() > glm::radians(135.0f) )
			camera_.setFOV(glm::radians(135.0f));
	}

};





