#pragma once
#include "All.hpp"
#include "GLObjects.hpp"
#include "ShaderBuilder.hpp"
#include "ShaderSource.hpp"
#include "TextureData.hpp"
#include "Transform.hpp"
#include "Vertex2D.hpp"
#include "GLObjectPool.hpp"

#include <algorithm>
#include <cstring>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <array>
#include <memory>
#include <utility>



class Sprite {
private:
    learn::Shared<learn::TextureHandle> texture_;
    glm::vec3 color_;

    friend class SpriteRenderer;

public:
    Sprite(learn::Shared<learn::TextureHandle> texture, glm::vec3 color)
        : texture_{ std::move(texture) }, color_{ color } {}

    explicit Sprite(learn::Shared<learn::TextureHandle> texture)
        : Sprite(std::move(texture), glm::vec3{ 1.0f, 1.0f, 1.0f }) {}

    const learn::TextureHandle& texture() const noexcept { return *texture_; }
    glm::vec3& color() noexcept { return color_; }
    const glm::vec3& color() const noexcept { return color_; }
};




class SpriteRenderer {
private:
    learn::VBO vbo_;
    learn::VAO vao_;
    learn::ShaderProgram sp_;

    constexpr static std::array<learn::Vertex2D, 6> quad{{
        {{ 0.0f, 1.0f }, { 0.0f, 1.0f }},
        {{ 1.0f, 0.0f }, { 1.0f, 0.0f }},
        {{ 0.0f, 0.0f }, { 0.0f, 0.0f }},
        {{ 0.0f, 1.0f }, { 0.0f, 1.0f }},
        {{ 1.0f, 1.0f }, { 1.0f, 1.0f }},
        {{ 1.0f, 0.0f }, { 1.0f, 0.0f }},
    }};
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
        auto asp = sp_.use();

        sprite.texture_->bind_to_unit(gl::GL_TEXTURE0);

        asp.uniform("model", transform.model())
            .uniform("color", sprite.color())
            .uniform("image", 0);

        vao_.bind()
            .draw_arrays(gl::GL_TRIANGLES, 0, quad.size())
            .unbind();
    }
};
