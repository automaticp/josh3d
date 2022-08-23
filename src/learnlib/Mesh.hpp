#pragma once
#include <vector>
#include <utility>
#include <memory>
#include <glbinding/gl/gl.h>
#include "GLObjects.hpp"
#include "Vertex.hpp"


namespace learn {



template<typename V>
class Mesh {
public:
    using tex_handle_t = std::shared_ptr<TextureHandle>;

private:
    std::vector<V> vertices_;
    std::vector<gl::GLuint> elements_;

    tex_handle_t diffuse_;
    tex_handle_t specular_;

    VBO vbo_;
    VAO vao_;
    EBO ebo_;

public:
    Mesh(std::vector<V> vertices, std::vector<gl::GLuint> elements,
         tex_handle_t diffuse, tex_handle_t specular)
        : vertices_{ std::move(vertices) }, elements_{ std::move(elements) },
        diffuse_{ std::move(diffuse) }, specular_{ std::move(specular) }
    {
        using namespace gl;

        auto bvao = vao_.bind();

        vbo_.bind()
           .attach_data(vertices_.size(), vertices_.data(), GL_STATIC_DRAW)
           .associate_with(bvao, VertexTraits<V>::aparams);

        ebo_.bind(bvao)
           .attach_data(elements_.size(), elements_.data(), GL_STATIC_DRAW);

    }

    void draw(ActiveShaderProgram& sp) {
        using namespace gl;

        sp.uniform("material.diffuse", 0);
	    diffuse_->bind_to_unit(GL_TEXTURE0);

        sp.uniform("material.specular", 1);
	    specular_->bind_to_unit(GL_TEXTURE1);

        sp.uniform("material.shininess", 128.0f);


        vao_.bind()
           .draw_elements(GL_TRIANGLES, elements_.size(), GL_UNSIGNED_INT, nullptr);


    }


};


} // namespace learn


