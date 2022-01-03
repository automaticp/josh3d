#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <tuple>
#include <numbers>
#include <utility>
#include <thread>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "TypeAliases.h"
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

std::tuple<int, int>  getWindowSize(GLFWwindow* window) {
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	return { w, h };
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

[[nodiscard]] 
GLFWwindow* initWindow(int width, int height, const std::string& name) {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window{ glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr) };
	if ( !window ) {
		glfwTerminate();
		throw std::runtime_error("runtime_error: failed to create GLFW window");
	}

	glfwMakeContextCurrent(window);

	return window;
}



int main() {

	// Init GLFW window
	int defaultWidth{ 800 };
	int defaultHeight{ 600 };
	GLFWwindow* window{ initWindow(defaultWidth, defaultHeight, "Window Name") };

	glfwSwapInterval(0);
	// Init GLAD
	if ( !gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) ) {
		throw std::runtime_error("runtime_error: failed to initialize GLAD");
	}
	glViewport(0, 0, defaultWidth, defaultHeight);

	// Window size callback
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);



	// Shaders
	VertexShader VS{ "VertexShader.vert" };

	// Textured Box
	FragmentShader FSTextured{ "TexturedObject.frag" };
	ShaderProgram SPTextured{ { VS, FSTextured } };

	// Colored Box
	FragmentShader FSColored{ "ColoredObject.frag" };
	ShaderProgram SPColored{ { VS, FSColored } };
	
	// Lighting Source
	FragmentShader FSLightSource{ "LightSource.frag" };
	ShaderProgram SPLightSource{ { VS, FSLightSource } };




	for ( Shader& shader : std::vector<refw<Shader>>{ VS, FSTextured, FSColored, FSLightSource } ) {
		std::cout << shader.getCompileInfo() << '\n';
		shader.destroy();
		std::cout << "Shader Id: " << shader.getId() << " is " << (shader ? std::string("alive") : std::string("deleted")) << '\n';
	}

	for ( ShaderProgram& sp : std::vector<refw<ShaderProgram>>{ SPTextured, SPColored, SPLightSource } ) {
		std::cout << sp.getLinkInfo() << '\n';
	}






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


	std::vector<glm::vec3> cubePositions{
		{0.0f,  0.0f,  0.0f},
		{2.0f,  5.0f, -15.0f},
		{-1.5f, -2.2f, -2.5f},
		{-3.8f, -2.0f, -12.3f},
		{2.4f, -0.4f, -3.5f},
		{-1.7f,  3.0f, -7.5f},
		{1.3f, -2.0f, -2.5f},
		{1.5f,  2.0f, -2.5f},
		{1.5f,  0.2f, -1.5f},
		{-1.3f,  1.0f, -1.5f}
	};

	// Creating VAO and linking data from VBO
	VBO boxVBO{ std::move(vertices), { { 0, 3 }, { 1, 3 }, { 2, 2 } } };
	
	VAO boxVAO{ boxVBO };
	VAO lightVAO{ boxVBO };



	// Loading textures
	//Texture texture1("container.jpg", GL_RGB);
	Texture texture1("arch.png", GL_RGB);
	Texture texture2("awesomeface.png", GL_RGBA);



	glEnable(GL_DEPTH_TEST);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	Camera cam{ glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
	InputFreeCamera input{ window, cam };

	
	// don't do this kek, input needs some internal timer
	// but this works
	// really fast
	/*
	std::thread t{ 
		[&input, &window]() {
			while ( !glfwWindowShouldClose(window) ) { 
				glfwPollEvents();
				input.processInput(); 
			}
		} 
	};
	*/
	
	glm::mat4 view{};
	glm::mat4 projection{};
	glm::mat4 model{};
	glm::mat3 normalModel{};

	while ( !glfwWindowShouldClose(window) ) {

		currentFrameTime = static_cast<float>(glfwGetTime());
		deltaFrameTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		auto [width, height] { getWindowSize(window) };

		input.processInput();





		projection = glm::perspective(cam.getFOV(), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
		view = cam.getViewMat();

		// Lighting Source
		SPLightSource.use();

		glm::vec3 lightColor{ 1.0f };
		glm::vec3 lightPos{ glm::sin(currentFrameTime), 3.0f, 2.0f * glm::cos(currentFrameTime) };
		//glm::vec3 lightPos{ -2.5f, -2.5f, 2.0f };
		glm::vec3 camPos{ cam.getPos() };

		model = glm::mat4{ 1.0f };
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3{ 0.2f });
		normalModel = glm::mat3(glm::transpose(glm::inverse(model)));
		SPLightSource.setUniform(SPLightSource.getUniformLocation("model"), model);
		SPLightSource.setUniform(SPLightSource.getUniformLocation("normalModel"), normalModel);
		SPLightSource.setUniform(SPLightSource.getUniformLocation("projection"), projection);
		SPLightSource.setUniform(SPLightSource.getUniformLocation("view"), view);
		SPLightSource.setUniform(SPLightSource.getUniformLocation("lightColor"), lightColor);

		lightVAO.bindAndDraw();

		// Textured Boxes
		SPTextured.use();

		texture1.setActiveUnitAndBind(0);
		texture2.setActiveUnitAndBind(1);
		SPTextured.setUniform(SPTextured.getUniformLocation("texture1"), 0);
		SPTextured.setUniform(SPTextured.getUniformLocation("texture2"), 1);

		for ( int i{ 0 }; i < cubePositions.size(); ++i ) {

			model = glm::mat4(1.0f);
			auto angle{ 20.0f * static_cast<float>(i) };

			model = glm::translate(model, cubePositions[i]);
			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.5f, 0.3f));
			normalModel = glm::mat3(glm::transpose(glm::inverse(model)));
			SPTextured.setUniform(SPTextured.getUniformLocation("model"), model);
			SPTextured.setUniform(SPTextured.getUniformLocation("normalModel"), normalModel);
			boxVAO.bindAndDraw();

		}

		SPTextured.setUniform(SPTextured.getUniformLocation("projection"), projection);
		SPTextured.setUniform(SPTextured.getUniformLocation("view"), view);
		SPTextured.setUniform(SPTextured.getUniformLocation("lightColor"), lightColor);
		SPTextured.setUniform(SPTextured.getUniformLocation("lightPos"), lightPos);
		SPTextured.setUniform(SPTextured.getUniformLocation("camPos"), camPos);
		


		// Colored Object
		SPColored.use();
		
		glm::vec3 objectColor{ 1.0f, 0.5f, 0.31f };

		model = glm::mat4{ 1.0f };
		model = glm::translate(model, glm::vec3{ 0.0f, 2.5f, 1.0f });
		model = glm::rotate(model, glm::radians(30.0f), glm::vec3{ 1.0f });
		model = glm::scale(model, glm::vec3{ 0.75f });
		normalModel = glm::mat3(glm::transpose(glm::inverse(model)));
		SPColored.setUniform(SPColored.getUniformLocation("model"), model);
		SPColored.setUniform(SPColored.getUniformLocation("normalModel"), normalModel);
		SPColored.setUniform(SPColored.getUniformLocation("projection"), projection);
		SPColored.setUniform(SPColored.getUniformLocation("view"), view);
		SPColored.setUniform(SPColored.getUniformLocation("objectColor"), objectColor);
		SPColored.setUniform(SPColored.getUniformLocation("lightColor"), lightColor);
		SPColored.setUniform(SPColored.getUniformLocation("lightPos"), lightPos);
		SPColored.setUniform(SPColored.getUniformLocation("camPos"), camPos);

		boxVAO.bindAndDraw();





		glfwSwapBuffers(window);
		glfwPollEvents();
	}



	//t.join();










	glfwTerminate();

	return 0;
}

