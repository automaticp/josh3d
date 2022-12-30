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
    learn::Transform transform_;
    glm::vec3 color_;

    friend class SpriteRenderer;

public:
    Sprite(learn::Shared<learn::TextureHandle> texture, const learn::Transform& transform, glm::vec3 color)
        : texture_{ std::move(texture) }, transform_{ transform }, color_{ color } {}

    Sprite(learn::Shared<learn::TextureHandle> texture, const learn::Transform& transform)
        : Sprite(std::move(texture), transform, glm::vec3{ 1.0f, 1.0f, 1.0f }) {}

    Sprite(learn::Shared<learn::TextureHandle> texture)
        : Sprite(std::move(texture), {}, glm::vec3{ 1.0f, 1.0f, 1.0f }) {}

    learn::Transform& transform() noexcept { return transform_; }
    const learn::Transform& transform() const noexcept { return transform_; }

    // learn::TextureHandle& texture() noexcept { return texture_; }
    glm::vec3& color() noexcept { return color_; }
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

    void draw_sprite(Sprite& sprite) {
        auto asp = sp_.use();

        sprite.texture_->bind_to_unit(gl::GL_TEXTURE0);

        asp.uniform("model", sprite.transform().model())
            .uniform("color", sprite.color())
            .uniform("image", 0); // Is this step right?

        vao_.bind()
            .draw_arrays(gl::GL_TRIANGLES, 0, quad.size())
            .unbind();
    }
};
