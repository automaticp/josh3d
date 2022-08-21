#pragma once
#include <vector>
#include <utility>
#include <memory>
#include <glbinding/gl/gl.h>
#include "GLObjects.hpp"
#include "Vertex.hpp"
#include "ShaderProgram.h"


namespace learn {

using namespace gl;

template<typename V>
class Mesh {
public:
    using tex_handle_t = std::shared_ptr<TextureHandle>;

private:
    std::vector<V> vertices_;
    std::vector<GLuint> elements_;

    tex_handle_t diffuse_;
    tex_handle_t specular_;

    VBO vbo_;
    VAO vao_;
    EBO ebo_;

public:
    Mesh(std::vector<V> vertices, std::vector<GLuint> elements,
         tex_handle_t diffuse, tex_handle_t specular)
        : vertices_{ std::move(vertices) }, elements_{ std::move(elements) },
        diffuse_{ std::move(diffuse) }, specular_{ std::move(specular) }
    {

        auto bvao = vao_.bind();

        vbo_.bind()
           .attach_data(vertices_.size(), vertices_.data(), GL_STATIC_DRAW)
           .associate_with(bvao, VertexTraits<V>::aparams);

        ebo_.bind(bvao)
           .attach_data(elements_.size(), elements_.data(), GL_STATIC_DRAW);

    }

    void draw(ShaderProgram& sp) {

        sp.setUniform("material.diffuse", 0);
	    diffuse_->bind_to_unit(GL_TEXTURE0);

        sp.setUniform("material.specular", 1);
	    specular_->bind_to_unit(GL_TEXTURE1);

        sp.setUniform("material.shininess", 128.0f);


        vao_.bind()
           .draw_elements(GL_TRIANGLES, elements_.size(), GL_UNSIGNED_INT, nullptr);


    }


};


} // namespace learn


