#include <numeric>
#include <glfwpp/glfwpp.h>
#include <glbinding/gl/gl.h>
#include "Scenes.hpp"
#include "All.hpp"


void render_model_scene(glfw::Window& window) {

	using namespace learn;
	using namespace gl;


	ShaderSource vert_src{ FileReader{}, "src/shaders/VertexShader.vert" };
	vert_src.find_and_insert_as_next_line(
		"uniform",
		"uniform float time;"
	);

	vert_src.find_and_replace(
		"texCoord = aTexCoord;",
		"texCoord = cos(time) * aTexCoord;"
	);

	ShaderProgram sp{
		ShaderBuilder()
			.add_vert(vert_src)
			.load_frag("src/shaders/TextureMaterialObject.frag")
			.get()
	};


	Assimp::Importer importer{};

	auto backpack_model {
		AssimpModelLoader(importer)
			.add_flags(aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph)
			.load("data/models/backpack/backpack.obj").get()
	};


	Camera cam{ glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
	// InputFreeCamera input{ window, cam };

	RebindableInputFreeCamera rinput{ window, cam };

	rinput.add_keybind(
		glfw::KeyCode::R,
		[&sp](const IInput::KeyCallbackArgs& args) {
			if ( args.state == glfw::KeyState::Release ) {
				sp = ShaderBuilder()
					.load_vert("src/shaders/VertexShader.vert")
					.load_frag("src/shaders/TextureMaterialObject.frag")
					.get();
			}
		}
	);

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

		glfw::pollEvents();
		rinput.process_input();

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

		asp.uniform("time", static_cast<float>(global_frame_timer.current()));

		backpack_model.draw(asp);


	}
}








// Vertex Data of a Cube {3: pos, 3: normals, 2: tex coord}
std::vector<learn::Vertex> vertices({
		{{-0.5f, -0.5f, -0.5f}, {+0.0f, +0.0f, -1.0f}, {+0.0f, +0.0f}},
		{{+0.5f, -0.5f, -0.5f}, {+0.0f, +0.0f, -1.0f}, {+1.0f, +0.0f}},
		{{+0.5f, +0.5f, -0.5f}, {+0.0f, +0.0f, -1.0f}, {+1.0f, +1.0f}},
		{{+0.5f, +0.5f, -0.5f}, {+0.0f, +0.0f, -1.0f}, {+1.0f, +1.0f}},
		{{-0.5f, +0.5f, -0.5f}, {+0.0f, +0.0f, -1.0f}, {+0.0f, +1.0f}},
		{{-0.5f, -0.5f, -0.5f}, {+0.0f, +0.0f, -1.0f}, {+0.0f, +0.0f}},
		{{-0.5f, -0.5f, +0.5f}, {+0.0f, +0.0f, +1.0f}, {+0.0f, +0.0f}},
		{{+0.5f, -0.5f, +0.5f}, {+0.0f, +0.0f, +1.0f}, {+1.0f, +0.0f}},
		{{+0.5f, +0.5f, +0.5f}, {+0.0f, +0.0f, +1.0f}, {+1.0f, +1.0f}},
		{{+0.5f, +0.5f, +0.5f}, {+0.0f, +0.0f, +1.0f}, {+1.0f, +1.0f}},
		{{-0.5f, +0.5f, +0.5f}, {+0.0f, +0.0f, +1.0f}, {+0.0f, +1.0f}},
		{{-0.5f, -0.5f, +0.5f}, {+0.0f, +0.0f, +1.0f}, {+0.0f, +0.0f}},
		{{-0.5f, +0.5f, +0.5f}, {-1.0f, +0.0f, +0.0f}, {+1.0f, +0.0f}},
		{{-0.5f, +0.5f, -0.5f}, {-1.0f, +0.0f, +0.0f}, {+1.0f, +1.0f}},
		{{-0.5f, -0.5f, -0.5f}, {-1.0f, +0.0f, +0.0f}, {+0.0f, +1.0f}},
		{{-0.5f, -0.5f, -0.5f}, {-1.0f, +0.0f, +0.0f}, {+0.0f, +1.0f}},
		{{-0.5f, -0.5f, +0.5f}, {-1.0f, +0.0f, +0.0f}, {+0.0f, +0.0f}},
		{{-0.5f, +0.5f, +0.5f}, {-1.0f, +0.0f, +0.0f}, {+1.0f, +0.0f}},
		{{+0.5f, +0.5f, +0.5f}, {+1.0f, +0.0f, +0.0f}, {+1.0f, +0.0f}},
		{{+0.5f, +0.5f, -0.5f}, {+1.0f, +0.0f, +0.0f}, {+1.0f, +1.0f}},
		{{+0.5f, -0.5f, -0.5f}, {+1.0f, +0.0f, +0.0f}, {+0.0f, +1.0f}},
		{{+0.5f, -0.5f, -0.5f}, {+1.0f, +0.0f, +0.0f}, {+0.0f, +1.0f}},
		{{+0.5f, -0.5f, +0.5f}, {+1.0f, +0.0f, +0.0f}, {+0.0f, +0.0f}},
		{{+0.5f, +0.5f, +0.5f}, {+1.0f, +0.0f, +0.0f}, {+1.0f, +0.0f}},
		{{-0.5f, -0.5f, -0.5f}, {+0.0f, -1.0f, +0.0f}, {+0.0f, +1.0f}},
		{{+0.5f, -0.5f, -0.5f}, {+0.0f, -1.0f, +0.0f}, {+1.0f, +1.0f}},
		{{+0.5f, -0.5f, +0.5f}, {+0.0f, -1.0f, +0.0f}, {+1.0f, +0.0f}},
		{{+0.5f, -0.5f, +0.5f}, {+0.0f, -1.0f, +0.0f}, {+1.0f, +0.0f}},
		{{-0.5f, -0.5f, +0.5f}, {+0.0f, -1.0f, +0.0f}, {+0.0f, +0.0f}},
		{{-0.5f, -0.5f, -0.5f}, {+0.0f, -1.0f, +0.0f}, {+0.0f, +1.0f}},
		{{-0.5f, +0.5f, -0.5f}, {+0.0f, +1.0f, +0.0f}, {+0.0f, +1.0f}},
		{{+0.5f, +0.5f, -0.5f}, {+0.0f, +1.0f, +0.0f}, {+1.0f, +1.0f}},
		{{+0.5f, +0.5f, +0.5f}, {+0.0f, +1.0f, +0.0f}, {+1.0f, +0.0f}},
		{{+0.5f, +0.5f, +0.5f}, {+0.0f, +1.0f, +0.0f}, {+1.0f, +0.0f}},
		{{-0.5f, +0.5f, +0.5f}, {+0.0f, +1.0f, +0.0f}, {+0.0f, +0.0f}},
		{{-0.5f, +0.5f, -0.5f}, {+0.0f, +1.0f, +0.0f}, {+0.0f, +1.0f}},
});




void render_cube_scene(glfw::Window& window) {


    using namespace learn;
    using namespace gl;

	ShaderProgram sp{
		ShaderBuilder()
			.load_vert("src/shaders/VertexShader.vert")
			.load_frag("src/shaders/MultiLightObject.frag")
			.get()
	};

	ShaderProgram sp_light{
		ShaderBuilder()
			.load_vert("src/shaders/VertexShader.vert")
			.load_frag("src/shaders/LightSource.frag")
			.get()
	};


    // ---- Boxes ----

    std::vector<GLuint> elements(vertices.size());
    std::iota(elements.begin(), elements.end(), 0u);

    Mesh<Vertex> box{
        vertices,
        elements,
        global_texture_handle_pool.load("data/textures/container2_d.png"),
        global_texture_handle_pool.load("data/textures/container2_colored_s.png")
    };

	std::vector<glm::vec3> cube_positions {
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


    // ---- Light Sources ----

    // Light Box
    VAO light_vao;
    auto bound_light_vao = light_vao.bind();
    VBO light_vbo;
    light_vbo.bind()
        .attach_data(vertices.size(), vertices.data(), GL_STATIC_DRAW)
        .associate_with(bound_light_vao, VertexTraits<Vertex>::aparams);
    // bound_light_vao.unbind();


    // Point
	std::vector<glm::vec3> point_light_positions {
			glm::vec3( 0.7f,  0.2f,  2.0f),
			glm::vec3( 2.3f, -3.3f, -4.0f),
			glm::vec3(-4.0f,  2.0f, -12.0f),
			glm::vec3( 0.0f,  0.0f, -3.0f)
	};

    std::vector<light::Point> lps;
    lps.reserve(point_light_positions.size());
    for (auto&& pos : point_light_positions) {
        lps.emplace_back(
            light::Point{
                .color = glm::vec3(1.0f, 1.0f, 0.8f),
                .position = pos,
                .attenuation = light::Attenuation{ .constant = 1.0f, .linear = 0.4f, .quadratic = 0.2f },
            }
        );
    }

    // Directional
    light::Directional ld{
        .color = { 0.3f, 0.3f, 0.2f },
        .direction = { -0.2f, -1.0f, -0.3f }
    };

    // Spotlight
    light::Spotlight ls{
        .color = glm::vec3(1.0f),
        .position = {},     // Update
        .direction = {},    // Update
        .attenuation = light::Attenuation{ .constant = 1.0f, .linear = 1.0f, .quadratic = 2.1f },
        .inner_cutoff_radians = glm::radians(12.0f),
        .outer_cutoff_radians = glm::radians(15.0f)
    };



	Camera cam{ glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
	InputFreeCamera input{ window, cam };

	glm::mat4 view{};
	glm::mat4 projection{};
	glm::mat4 model{};
	glm::mat3 normal_model{};

	while ( !window.shouldClose() ) {

        global_frame_timer.update();

		window.swapBuffers();
		glClearColor(0.15f, 0.15f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		input.process_input();
		glfw::pollEvents();


		auto [width, height] = window.getSize();
		projection = glm::perspective(cam.get_fov(), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
		view = cam.view_mat();

		glm::vec3 cam_pos{ cam.get_pos() };


		// Textured diff+spec objects with multiple of lights shining on them

		// boxVAO.bind();

		ActiveShaderProgram asp{ sp.use() };
        asp .uniform("projection", projection)
		    .uniform("view", view)
		    .uniform("camPos", cam_pos);


        // ---- Light Sources ----

        // Directional
		asp .uniform("dirLight.color", ld.color)
		    .uniform("dirLight.direction", ld.direction);


        // Point
		for (size_t i{ 0 }; i < 4; ++i) {
			// what a mess, though
			std::string col_name{ "pointLights[x].color" };
			std::string pos_name{ "pointLights[x].position" };
			std::string att0_name{ "pointLights[x].attenuation.constant" };
			std::string att1_name{ "pointLights[x].attenuation.linear" };
			std::string att2_name{ "pointLights[x].attenuation.quadratic" };
			std::string i_string{ std::to_string(i) };
			col_name.replace(12, 1, i_string);
			pos_name.replace(12, 1, i_string);
			att0_name.replace(12, 1, i_string);
			att1_name.replace(12, 1, i_string);
			att2_name.replace(12, 1, i_string);

			asp.uniform(col_name, lps[i].color);
			asp.uniform(pos_name, lps[i].position);
			asp.uniform(att0_name, lps[i].attenuation.constant);
			asp.uniform(att1_name, lps[i].attenuation.linear);
			asp.uniform(att2_name, lps[i].attenuation.quadratic);
		}


		// Spotlight
        ls.position = cam_pos;
        ls.direction = -cam.back_uv();
		asp.uniform("spotLight.color", ls.color);
		asp.uniform("spotLight.position", ls.position);
		asp.uniform("spotLight.direction", ls.direction);
		asp.uniform("spotLight.attenuation.constant", ls.attenuation.constant);
		asp.uniform("spotLight.attenuation.linear", ls.attenuation.linear);
		asp.uniform("spotLight.attenuation.quadratic", ls.attenuation.quadratic);
		asp.uniform("spotLight.innerCutoffCos", glm::cos(ls.inner_cutoff_radians));
		asp.uniform("spotLight.outerCutoffCos", glm::cos(ls.outer_cutoff_radians));



		// ---- Scene of Boxes ----

		for (size_t i{0}; i < 10; ++i) {
			model = glm::mat4{ 1.0f };
			model = glm::translate(model, cube_positions[i]);
			float angle = 20.0f * i;

			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
			asp.uniform("model", model);

			normal_model = glm::mat3(glm::transpose(glm::inverse(model)));
			asp.uniform("normalModel", normal_model);


			box.draw(asp);
		}



		// Point Lighting Sources

		ActiveShaderProgram asp_light{ sp_light.use() };

        auto bound_light_vao = light_vao.bind();

		asp_light.uniform("projection", projection);
		asp_light.uniform("view", view);

		for (auto&& lp : lps) {
			model = glm::mat4{ 1.0f };
			model = glm::translate(model, lp.position);
			model = glm::scale(model, glm::vec3{ 0.2f });
			asp_light.uniform("model", model);

			normal_model = glm::mat3(glm::transpose(glm::inverse(model)));
			asp_light.uniform("normalModel", normal_model);

			asp_light.uniform("lightColor", lp.color);

            bound_light_vao.draw_arrays(GL_TRIANGLES, 0, vertices.size());
		}


	}


}

