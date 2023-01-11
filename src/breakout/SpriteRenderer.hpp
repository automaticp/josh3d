#pragma once
#include "GLObjects.hpp"
#include "ShaderSource.hpp"
#include "ShaderBuilder.hpp"
#include "Vertex2D.hpp"
#include "Sprite.hpp"
#include "Transform.hpp"


class SpriteRenderer {
private:
    learn::VBO vbo_;
    learn::VAO vao_;
    learn::ShaderProgram sp_;

    constexpr static std::array<learn::Vertex2D, 6> quad{ {
        { { -0.5f, +0.5f }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f }, { 0.0f, 0.0f } },
        { { +0.5f, -0.5f }, { 1.0f, 0.0f } },

        { { +0.5f, +0.5f }, { 1.0f, 1.0f } },
        { { -0.5f, +0.5f }, { 0.0f, 1.0f } },
        { { +0.5f, -0.5f }, { 1.0f, 0.0f } },
    } };

public:
    SpriteRenderer(const learn::ShaderSource& vert, const learn::ShaderSource& frag)
        : sp_{
            learn::ShaderBuilder()
                .add_vert(vert)
                .add_frag(frag)
                .get()
        }
    {
        vbo_.bind()
            .attach_data(quad.size(), quad.data(), gl::GL_STATIC_DRAW)
            .associate_with<learn::Vertex2D>(vao_.bind());
    }


    learn::ShaderProgram& shader() noexcept { return sp_; }

    void draw_sprite(const Sprite& sprite, const learn::Transform& transform) {
        draw_sprite(sprite, transform, sprite.color());
    }

    void draw_sprite(const Sprite& sprite, const learn::Transform& transform, glm::vec4 color) {
        auto asp = sp_.use();

        sprite.texture_->bind_to_unit(gl::GL_TEXTURE0);

        asp.uniform("model", transform.model())
            .uniform("color", color)
            .uniform("image", 0);

        vao_.bind()
            .draw_arrays(gl::GL_TRIANGLES, 0, quad.size())
            .unbind();
    }
};

