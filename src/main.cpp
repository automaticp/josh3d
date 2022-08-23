#include <vector>
#include <cmath>
#include <numbers>
#include <memory>
#include <iostream>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <glfwpp/glfwpp.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "All.hpp"

static void render_cube_scene(glfw::Window&);
static void render_model_scene(glfw::Window&);

int main() {
	using namespace gl;
	using namespace learn;

	// Init GLFW and create a window
	auto glfw_instance{ glfw::init() };

	glfw::WindowHints{
		.contextVersionMajor=3, .contextVersionMinor=3, .openglProfile=glfw::OpenGlProfile::Core
	}.apply();
	glfw::Window window{ 800, 600, "WindowName" };
	window.framebufferSizeEvent.setCallback([](glfw::Window& window, int w, int h) { glViewport(0, 0, w, h); });
	glfw::makeContextCurrent(window);
	glfw::swapInterval(0);
	window.setInputModeCursor(glfw::CursorMode::Disabled);

	// Init glbindings
	glbinding::initialize(glfwGetProcAddress);
#ifndef NDEBUG
	enable_glbinding_logger();
#endif
	auto [width, height] { window.getSize() };
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);


	// render_cube_scene(window);
	render_model_scene(window);

	return 0;
}


static void render_model_scene(glfw::Window& window) {

	using namespace learn;
	using namespace gl;

	FileReader fr;

	VertexShader vs;
	vs.set_source(fr("data/shaders/VertexShader.vert")).compile();

	FragmentShader fs;
	fs.set_source(fr("data/shaders/TextureMaterialObject.frag")).compile();

	ShaderProgram sp;
	sp.attach_shader(vs).attach_shader(fs).link();


	Assimp::Importer importer{};

	auto backpack_model {
		AssimpModelLoader(importer)
			.add_flags(aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph)
			.load("data/models/backpack/backpack.obj").get()
	};


	Camera cam{ glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
	InputFreeCamera input{ window, cam };

	glm::mat4 view{};
	glm::mat4 projection{};
	glm::mat4 model{};
	glm::mat3 normal_model{};

	light::Point lp{
		.color = { 0.3f, 0.3f, 0.2f },
		.position = { 0.5f, 0.8f, 1.5f },
	};

	while ( !window.shouldClose() ) {

		global_frame_timer.update();
		window.swapBuffers();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		input.process_input();
		glfw::pollEvents();

		// Get projection and view matricies from camera positions
		auto [width, height] = window.getSize();
		projection = glm::perspective(cam.get_fov(), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
		view = cam.view_mat();

		glm::vec3 cam_pos{ cam.get_pos() };

		auto asp = sp.use();
		asp.uniform("projection", projection);
		asp.uniform("view", view);
		asp.uniform("camPos", cam_pos);

		model = glm::mat4{ 1.0f };
		asp.uniform("model", model);

		normal_model = glm::mat3(glm::transpose(glm::inverse(model)));
		asp.uniform("normalModel", normal_model);


		asp.uniform("lightColor", lp.color);
		asp.uniform("lightPos", lp.position);

		backpack_model.draw(asp);


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

/*
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
	SPMultiLight.uniform("material.diffuse", 0);
	SPMultiLight.uniform("material.specular", 1);
	SPMultiLight.uniform("material.shininess", 128.0f);

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
		input.process_input();
		glfw::pollEvents();

		// Get projection and view matricies from camera positions
		auto [width, height] = window.getSize();
		projection = glm::perspective(cam.get_fov(), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
		view = cam.view_mat();

		glm::vec3 camPos{ cam.get_pos() };


		// Textured diff+spec objects with multiple of lights shining on them
		SPMultiLight.use();

		boxVAO.bind();

		SPMultiLight.uniform("projection", projection);
		SPMultiLight.uniform("view", view);
		SPMultiLight.uniform("camPos", camPos);

		// ---- Light Sources ----
		// (most of them are constant, but I'll set them within the render loop anyway, for clarity)

		// Directional
		light::Directional ld{
			.color = { 0.3f, 0.3f, 0.2f },
			.direction = { -0.2f, -1.0f, -0.3f }
		};
		SPMultiLight.uniform("dirLight.color", ld.color);
		SPMultiLight.uniform("dirLight.direction", ld.direction);


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

			SPMultiLight.uniform(colName.c_str(), lps[i].color);
			SPMultiLight.uniform(posName.c_str(), lps[i].position);
			SPMultiLight.uniform(att0Name.c_str(), lps[i].attenuation.constant);
			SPMultiLight.uniform(att1Name.c_str(), lps[i].attenuation.linear);
			SPMultiLight.uniform(att2Name.c_str(), lps[i].attenuation.quadratic);
		}


		// Spotlight
		light::Spotlight ls{
				.color = glm::vec3(1.0f),
				.position = camPos,
				.direction = -cam.back_uv(),
				.attenuation = light::Attenuation{ .constant = 1.0f, .linear = 1.0f, .quadratic = 2.1f },
				.innerCutoffRad = glm::radians(12.0f),
				.outerCutoffRad = glm::radians(15.0f)
		};
		SPMultiLight.uniform("spotLight.color", ls.color);
		SPMultiLight.uniform("spotLight.position", ls.position);
		SPMultiLight.uniform("spotLight.direction", ls.direction);
		SPMultiLight.uniform("spotLight.attenuation.constant", ls.attenuation.constant);
		SPMultiLight.uniform("spotLight.attenuation.linear", ls.attenuation.linear);
		SPMultiLight.uniform("spotLight.attenuation.quadratic", ls.attenuation.quadratic);
		SPMultiLight.uniform("spotLight.innerCutoffCos", glm::cos(ls.innerCutoffRad));
		SPMultiLight.uniform("spotLight.outerCutoffCos", glm::cos(ls.outerCutoffRad));


		// ---- Scene of Boxes ----

		for (size_t i{0}; i < 10; ++i) {
			model = glm::mat4{ 1.0f };
			model = glm::translate(model, cubePositions[i]);
			float angle = 20.0f * i;

			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
			SPMultiLight.uniform("model", model);

			normalModel = glm::mat3(glm::transpose(glm::inverse(model)));
			SPMultiLight.uniform("normalModel", normalModel);


			boxVAO.draw();
		}



		// Point Lighting Sources

		SPLightSource.use();

		lightVAO.bind();


		SPLightSource.uniform("projection", projection);
		SPLightSource.uniform("view", view);


		for (size_t i{0}; i < 4; ++i) {
			model = glm::mat4{ 1.0f };
			model = glm::translate(model, pointLightPositions[i]);
			model = glm::scale(model, glm::vec3{ 0.2f });
			SPLightSource.uniform("model", model);

			normalModel = glm::mat3(glm::transpose(glm::inverse(model)));
			SPLightSource.uniform("normalModel", normalModel);

			SPLightSource.uniform("lightColor", lps[i].color);

			lightVAO.draw();
		}


	}


}
 */
