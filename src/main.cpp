#include <vector>
#include <cmath>
#include <numbers>
#include <memory>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <glfwpp/glfwpp.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Basis.h"
#include "Camera.h"
#include "Input.h"
#include "VBO.h"
#include "VAO.h"
#include "LightCasters.h"

#include "Mesh.h"

using namespace gl;

float currentFrameTime{};
float lastFrameTime{};
float deltaFrameTime{};

const OrthonormalBasis3D globalBasis{ glm::vec3(1.0f, 0.0, 0.0), glm::vec3(0.0f, 1.0f, 0.0f), true };


void updateFrameTime() {
	currentFrameTime = static_cast<float>(glfwGetTime());
	deltaFrameTime = currentFrameTime - lastFrameTime;
	lastFrameTime = currentFrameTime;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

static void render_cube_scene(glfw::Window&);
static void render_model_scene(glfw::Window&);

int main() {

	// Init GLFW and create a window
	auto glfwInstance{ glfw::init() };

	glfw::WindowHints{
		.contextVersionMajor=3, .contextVersionMinor=3, .openglProfile=glfw::OpenGlProfile::Core
	}.apply();
	glfw::Window window{ 800, 600, "WindowName" };
	window.framebufferSizeEvent.setCallback(framebufferSizeCallback);
	glfw::makeContextCurrent(window);
	glfw::swapInterval(0);
	window.setInputModeCursor(glfw::CursorMode::Disabled);

	// Init glbindings
	glbinding::initialize(glfwGetProcAddress);

	auto [width, height] { window.getSize() };
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);

	// render_cube_scene(window);
	render_model_scene(window);

	return 0;
}


static void render_model_scene(glfw::Window& window) {

	VertexShader vs{ "VertexShader.vert" };
	FragmentShader fs_model{ "MultiLightObject.frag" };
	FragmentShader fs_light{ "LightSource.frag" };

	ShaderProgram sp_model{ {vs, fs_model} };
	ShaderProgram sp_light{ {vs, fs_light} };


	Assimp::Importer importer{};
	// FIXME: fill path later
	Model<Vertex> backpack_model{ importer, "data/models/backpack/backpack.obj" };


	Camera cam{ glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
	InputFreeCamera input{ window, cam };

	glm::mat4 view{};
	glm::mat4 projection{};
	glm::mat4 model{};
	glm::mat3 normal_model{};


	while ( !window.shouldClose() ) {

		updateFrameTime();
		window.swapBuffers();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		input.processInput();
		glfw::pollEvents();

		// Get projection and view matricies from camera positions
		auto [width, height] = window.getSize();
		projection = glm::perspective(cam.getFOV(), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
		view = cam.getViewMat();

		glm::vec3 cam_pos{ cam.getPos() };

		sp_model.use();
		sp_model.setUniform("projection", projection);
		sp_model.setUniform("view", view);
		sp_model.setUniform("camPos", cam_pos);

		model = glm::mat4{ 1.0f };
		sp_model.setUniform("model", model);

		normal_model = glm::mat3(glm::transpose(glm::inverse(model)));
		sp_model.setUniform("normalModel", normal_model);

		backpack_model.draw(sp_model);


		// Directional Light
		light::Directional ld{
			.color = { 0.3f, 0.3f, 0.2f },
			.direction = { -0.2f, -1.0f, -0.3f }
		};
		sp_model.setUniform("dirLight.color", ld.color);
		sp_model.setUniform("dirLight.direction", ld.direction);


	}
}


// Vertex Data of a Cube {3: pos, 3: normals, 2: tex coord}
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


static void render_cube_scene(glfw::Window& window) {

	// Shaders
	VertexShader VS{ "VertexShader.vert" };
	// Texture Material Box
	FragmentShader FSMultiLight{ "MultiLightObject.frag" };
	ShaderProgram SPMultiLight{{ VS, FSMultiLight }};
	// Lighting Source
	FragmentShader FSLightSource{ "LightSource.frag" };
	ShaderProgram SPLightSource{ { VS, FSLightSource } };

	// Textures
	Texture boxTexDiffuse{ "container2_d.png" };
	Texture boxTexSpecular{ "container2_colored_s.png" };


	SPMultiLight.use();
	SPMultiLight.setUniform("material.diffuse", 0);
	SPMultiLight.setUniform("material.specular", 1);
	SPMultiLight.setUniform("material.shininess", 128.0f);

	boxTexDiffuse.setActiveUnitAndBind(0);
	boxTexSpecular.setActiveUnitAndBind(1);

	// Creating VAO and linking data from VBO
	VBO boxVBO{ vertices, { { 0, 3 }, { 1, 3 }, { 2, 2 } } };
	VAO boxVAO{ boxVBO };
	VAO lightVAO{ boxVBO };

	// Cube positions for the scene
	std::vector<glm::vec3> cubePositions {
			glm::vec3( 0.0f,  0.0f,  0.0f),
			glm::vec3( 2.0f,  5.0f, -15.0f),
			glm::vec3(-1.5f, -2.2f, -2.5f),
			glm::vec3(-3.8f, -2.0f, -12.3f),
			glm::vec3( 2.4f, -0.4f, -3.5f),
			glm::vec3(-1.7f,  3.0f, -7.5f),
			glm::vec3( 1.3f, -2.0f, -2.5f),
			glm::vec3( 1.5f,  2.0f, -2.5f),
			glm::vec3( 1.5f,  0.2f, -1.5f),
			glm::vec3(-1.3f,  1.0f, -1.5f)
	};

	std::vector<glm::vec3> pointLightPositions {
			glm::vec3( 0.7f,  0.2f,  2.0f),
			glm::vec3( 2.3f, -3.3f, -4.0f),
			glm::vec3(-4.0f,  2.0f, -12.0f),
			glm::vec3( 0.0f,  0.0f, -3.0f)
	};


	// Creating camera and binding it to input
	Camera cam{ glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
	InputFreeCamera input{ window, cam };

	// Transformation matrices
	glm::mat4 view{};
	glm::mat4 projection{};
	glm::mat4 model{};
	glm::mat3 normalModel{};

	while ( !window.shouldClose() ) {
		// Updates currentFrameTime, lastFrameTime, deltaFrameTime
		updateFrameTime();

		// Swap buffers first (back -> front), then clear the backbuffer for a new frame
		window.swapBuffers();
		glClearColor(0.15f, 0.15f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Process input
		input.processInput();
		glfw::pollEvents();

		// Get projection and view matricies from camera positions
		auto [width, height] = window.getSize();
		projection = glm::perspective(cam.getFOV(), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
		view = cam.getViewMat();

		glm::vec3 camPos{ cam.getPos() };


		// Textured diff+spec objects with multiple of lights shining on them
		SPMultiLight.use();

		boxVAO.bind();

		SPMultiLight.setUniform("projection", projection);
		SPMultiLight.setUniform("view", view);
		SPMultiLight.setUniform("camPos", camPos);

		// ---- Light Sources ----
		// (most of them are constant, but I'll set them within the render loop anyway, for clarity)

		// Directional
		light::Directional ld{
			.color = { 0.3f, 0.3f, 0.2f },
			.direction = { -0.2f, -1.0f, -0.3f }
		};
		SPMultiLight.setUniform("dirLight.color", ld.color);
		SPMultiLight.setUniform("dirLight.direction", ld.direction);


		// Point
		std::vector<light::Point> lps;
		lps.reserve(4);
		for (int i{ 0 }; i < 4; ++i) {
			lps.emplace_back(light::Point{
					.color = glm::vec3(1.0f, 1.0f, 0.8f),
					.position = pointLightPositions[i],
					.attenuation = light::Attenuation{ .constant = 1.0f, .linear = 0.4f, .quadratic = 0.2f },
			});
		}

		for (int i{ 0 }; i < 4; ++i) {
			// what a mess, though
			std::string colName{ "pointLights[x].color" };
			std::string posName{ "pointLights[x].position" };
			std::string att0Name{ "pointLights[x].attenuation.constant" };
			std::string att1Name{ "pointLights[x].attenuation.linear" };
			std::string att2Name{ "pointLights[x].attenuation.quadratic" };
			auto iString {std::to_string(i)};
			colName.replace(12, 1, iString);
			posName.replace(12, 1, iString);
			att0Name.replace(12, 1, iString);
			att1Name.replace(12, 1, iString);
			att2Name.replace(12, 1, iString);

			SPMultiLight.setUniform(colName.c_str(), lps[i].color);
			SPMultiLight.setUniform(posName.c_str(), lps[i].position);
			SPMultiLight.setUniform(att0Name.c_str(), lps[i].attenuation.constant);
			SPMultiLight.setUniform(att1Name.c_str(), lps[i].attenuation.linear);
			SPMultiLight.setUniform(att2Name.c_str(), lps[i].attenuation.quadratic);
		}


		// Spotlight
		light::Spotlight ls{
				.color = glm::vec3(1.0f),
				.position = camPos,
				.direction = -cam.backUV(),
				.attenuation = light::Attenuation{ .constant = 1.0f, .linear = 1.0f, .quadratic = 2.1f },
				.innerCutoffRad = glm::radians(12.0f),
				.outerCutoffRad = glm::radians(15.0f)
		};
		SPMultiLight.setUniform("spotLight.color", ls.color);
		SPMultiLight.setUniform("spotLight.position", ls.position);
		SPMultiLight.setUniform("spotLight.direction", ls.direction);
		SPMultiLight.setUniform("spotLight.attenuation.constant", ls.attenuation.constant);
		SPMultiLight.setUniform("spotLight.attenuation.linear", ls.attenuation.linear);
		SPMultiLight.setUniform("spotLight.attenuation.quadratic", ls.attenuation.quadratic);
		SPMultiLight.setUniform("spotLight.innerCutoffCos", glm::cos(ls.innerCutoffRad));
		SPMultiLight.setUniform("spotLight.outerCutoffCos", glm::cos(ls.outerCutoffRad));


		// ---- Scene of Boxes ----

		for (size_t i{0}; i < 10; ++i) {
			model = glm::mat4{ 1.0f };
			model = glm::translate(model, cubePositions[i]);
			float angle = 20.0f * i;

			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
			SPMultiLight.setUniform("model", model);

			normalModel = glm::mat3(glm::transpose(glm::inverse(model)));
			SPMultiLight.setUniform("normalModel", normalModel);


			boxVAO.draw();
		}



		// Point Lighting Sources

		SPLightSource.use();

		lightVAO.bind();


		SPLightSource.setUniform("projection", projection);
		SPLightSource.setUniform("view", view);


		for (size_t i{0}; i < 4; ++i) {
			model = glm::mat4{ 1.0f };
			model = glm::translate(model, pointLightPositions[i]);
			model = glm::scale(model, glm::vec3{ 0.2f });
			SPLightSource.setUniform("model", model);

			normalModel = glm::mat3(glm::transpose(glm::inverse(model)));
			SPLightSource.setUniform("normalModel", normalModel);

			SPLightSource.setUniform("lightColor", lps[i].color);

			lightVAO.draw();
		}


	}


}
