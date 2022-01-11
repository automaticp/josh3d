#include <vector>
#include <cmath>
#include <numbers>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GLFWWindowWrapper.h"
#include "Shader.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Basis.h"
#include "Camera.h"
#include "Input.h"
#include "VBO.h"
#include "VAO.h"

float currentFrameTime{};
float lastFrameTime{};
float deltaFrameTime{};

const OrthonormalBasis3D globalBasis{ glm::vec3(1.0f, 0.0, 0.0), glm::vec3(0.0f, 1.0f, 0.0f), true };

// Vertex Data {3: pos, 3: normals, 2: tex coord}
std::vector<float> vertices{
		-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

		-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f

};

void updateFrameTime() {
	currentFrameTime = static_cast<float>(glfwGetTime());
	deltaFrameTime = currentFrameTime - lastFrameTime;
	lastFrameTime = currentFrameTime;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

int main() {

	// Init GLFW window
	GLFWWindowWrapper window{ 800, 600, "Window Name", 3, 3, GLFWOpenGLProfile::core };
	window.setFramebufferSizeCallback(framebufferSizeCallback);
	glfwSwapInterval(0);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Init GLAD
	if ( !gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) ) {
		throw std::runtime_error("runtime_error: failed to initialize GLAD");
	}
	WindowSize windowSize { window.getWindowSize() };
	glViewport(0, 0, windowSize.width, windowSize.height);
	glEnable(GL_DEPTH_TEST);


	// Shaders
	VertexShader VS{ "VertexShader.vert" };
	// Colored Box
	FragmentShader FSColored{ "ColoredObject.frag" };
	ShaderProgram SPColored{ { VS, FSColored } };
	// Lighting Source
	FragmentShader FSLightSource{ "LightSource.frag" };
	ShaderProgram SPLightSource{ { VS, FSLightSource } };


	// Creating VAO and linking data from VBO
	VBO boxVBO{ vertices, { { 0, 3 }, { 1, 3 }, { 2, 2 } } };
	VAO boxVAO{ boxVBO };
	VAO lightVAO{ boxVBO };

	// Creating camera and binding it to input
	Camera cam{ glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
	InputFreeCamera input{ window, cam };

	// Transformation matrices
	glm::mat4 view{};
	glm::mat4 projection{};
	glm::mat4 model{};
	glm::mat3 normalModel{};

	while ( !glfwWindowShouldClose(window) ) {
		// Updates currentFrameTime, lastFrameTime, deltaFrameTime
		updateFrameTime();

		// Swap buffers first (back -> front), then clear the backbuffer for a new frame
		window.swapBuffers();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Process input
		input.processInput();
		glfwPollEvents();

		// Get projection and view matricies from camera positions
		windowSize = window.getWindowSize();
		projection = glm::perspective(cam.getFOV(), static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height), 0.1f, 100.0f);
		view = cam.getViewMat();

		// Lighting Source
		SPLightSource.use();

		glm::vec3 lightColor{ 1.0f };
		//glm::vec3 lightPos{ glm::sin(currentFrameTime), 3.0f, 2.0f * glm::cos(currentFrameTime) };
		glm::vec3 lightPos{ -2.0f, -0.5f, 1.5f };
		glm::vec3 camPos{ cam.getPos() };

		model = glm::mat4{ 1.0f };
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3{ 0.2f });
		normalModel = glm::mat3(glm::transpose(glm::inverse(model)));
		SPLightSource.setUniform("model", model);
		SPLightSource.setUniform("normalModel", normalModel);
		SPLightSource.setUniform("projection", projection);
		SPLightSource.setUniform("view", view);
		SPLightSource.setUniform("lightColor", lightColor);

		lightVAO.bindAndDraw();

		// Colored Object
		SPColored.use();
		
		glm::vec3 objectColor{ 1.0f, 0.5f, 0.31f };

		model = glm::mat4{ 1.0f };
		model = glm::translate(model, glm::vec3{ 0.0f, 2.5f, 1.0f });
		model = glm::rotate(model, glm::radians(30.0f), glm::vec3{ 1.0f });
		model = glm::scale(model, glm::vec3{ 0.75f });
		normalModel = glm::mat3(glm::transpose(glm::inverse(model)));

		SPColored.setUniform("model", model);
		SPColored.setUniform("normalModel", normalModel);
		SPColored.setUniform("projection", projection);
		SPColored.setUniform("view", view);
		SPColored.setUniform("objectColor", objectColor);
		SPColored.setUniform("lightColor", lightColor);
		SPColored.setUniform("lightPos", lightPos);
		SPColored.setUniform("camPos", camPos);

		boxVAO.bindAndDraw();

	}

	return 0;
}

