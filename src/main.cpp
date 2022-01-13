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
	// Material Box
	FragmentShader FSMaterial{ "MaterialObject.frag" };
	ShaderProgram SPMaterial{ { VS, FSMaterial } };
	// Texture Material Box
	FragmentShader FSTexture{ "TextureMaterialObject.frag" };
	ShaderProgram SPTexture{ {VS, FSTexture} };
	// Lighting Source
	FragmentShader FSLightSource{ "LightSource.frag" };
	ShaderProgram SPLightSource{ { VS, FSLightSource } };

	// Textures
	Texture boxTexDiffuse{ "container2_d.png", GL_RGBA };
	Texture boxTexSpecular{ "container2_s.png", GL_RGBA };

	SPTexture.use();
	SPTexture.setUniform("material.diffuse", 0);
	SPTexture.setUniform("material.specular", 1);
	SPTexture.setUniform("material.shininess", 128.0f);

	boxTexDiffuse.setActiveUnitAndBind(0);
	boxTexSpecular.setActiveUnitAndBind(1);

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
//		glm::vec3 lightColor{
//				(1.0f + glm::vec3{glm::sin(0.5f * currentFrameTime), glm::sin(currentFrameTime), glm::sin(2.0f * currentFrameTime)}) / 2.0f
//		};
//		//glm::vec3 lightPos{ glm::sin(currentFrameTime), 3.0f, 2.0f * glm::cos(currentFrameTime) };
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

		// Textured diff+spec object
		SPTexture.use();

		model = glm::mat4{ 1.0f };
		model = glm::translate(model, glm::vec3{ -2.0f, 1.5f, 0.5f });
		model = glm::rotate(model, glm::radians(-60.0f), glm::vec3{ 1.0f });
		model = glm::scale(model, glm::vec3{ 0.75f });
		normalModel = glm::mat3(glm::transpose(glm::inverse(model)));

		SPTexture.setUniform("model", model);
		SPTexture.setUniform("normalModel", normalModel);
		SPTexture.setUniform("projection", projection);
		SPTexture.setUniform("view", view);

		SPTexture.setUniform("lightColor", lightColor);
		SPTexture.setUniform("lightPos", lightPos);
		SPTexture.setUniform("camPos", camPos);


//		boxTexDiffuse.setActiveUnitAndBind(2);
//		boxTexSpecular.setActiveUnitAndBind(3);

//		SPTexture.setUniform("material.diffuse", static_cast<GLenum>(0));
//		SPTexture.setUniform("material.specular", static_cast<GLenum>(1));
//		SPTexture.setUniform("material.shininess", 128.0f);


		boxVAO.bindAndDraw();



		// Material Object
		SPMaterial.use();

		struct Material {
			glm::vec3 ambient;
			glm::vec3 diffuse;
			glm::vec3 specular;
			float shininess;
		};

		Material objectMat{ .ambient = {1.0f, 0.5f, 0.31f},
							.diffuse = {1.0f, 0.5f, 0.31f},
							.specular = {0.5f, 0.5f, 0.5f},
							.shininess = 32.0f
		};

		model = glm::mat4{ 1.0f };
		model = glm::translate(model, glm::vec3{ 0.0f, 2.5f, 1.0f });
		model = glm::rotate(model, glm::radians(30.0f), glm::vec3{ 1.0f });
		model = glm::scale(model, glm::vec3{ 0.75f });
		normalModel = glm::mat3(glm::transpose(glm::inverse(model)));

		SPMaterial.setUniform("model", model);
		SPMaterial.setUniform("normalModel", normalModel);
		SPMaterial.setUniform("projection", projection);
		SPMaterial.setUniform("view", view);

		SPMaterial.setUniform("material.ambient", objectMat.ambient);
		SPMaterial.setUniform("material.diffuse", objectMat.diffuse);
		SPMaterial.setUniform("material.specular", objectMat.specular);
		SPMaterial.setUniform("material.shininess", objectMat.shininess);

		SPMaterial.setUniform("lightColor", lightColor);
		SPMaterial.setUniform("lightPos", lightPos);
		SPMaterial.setUniform("camPos", camPos);

		boxVAO.bindAndDraw();

	}

	return 0;
}

