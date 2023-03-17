#pragma once
#include "GLObjects.hpp"
#include "Vertex2D.hpp"
#include "Transform2D.hpp"
#include "Shared.hpp"
#include "ShaderBuilder.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/gl.h>
#include <entt/entt.hpp>

namespace zdepth {
    static constexpr float near{ -1.0f };
    static constexpr float far { +1.0f };
    static constexpr float background{ -0.5f };
    static constexpr float foreground{ +0.5f };
};

struct Sprite {
    learn::Shared<learn::TextureHandle> texture;
    float depth{ zdepth::foreground };
    glm::vec4 color{ 1.f, 1.f, 1.f, 1.f };
};



class SpriteRenderSystem {
private:
    struct UniformLocations {
        gl::GLint projection{ -1 };
        gl::GLint model{ -1 };
        gl::GLint color{ -1 };
        gl::GLint image{ -1 };
    };


    learn::VBO vbo_;
    learn::VAO vao_;
    learn::ShaderProgram sp_;
    UniformLocations ulocs_;

    constexpr static std::array<learn::Vertex2D, 6> quad{ {
        { { -0.5f, +0.5f }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f }, { 0.0f, 0.0f } },
        { { +0.5f, -0.5f }, { 1.0f, 0.0f } },

        { { +0.5f, +0.5f }, { 1.0f, 1.0f } },
        { { -0.5f, +0.5f }, { 0.0f, 1.0f } },
        { { +0.5f, -0.5f }, { 1.0f, 0.0f } },
    } };

public:
    SpriteRenderSystem(const glm::mat4& projection)
        : sp_{
            learn::ShaderBuilder()
                .load_vert("src/shaders/sprite.vert")
                .load_frag("src/shaders/sprite.frag")
                .get()
        }
        , ulocs_{
            .projection = sp_.location_of("projection"),
            .model = sp_.location_of("model"),
            .color = sp_.location_of("color"),
            .image = sp_.location_of("image")
        }
    {
        using namespace gl;

        sp_.use()
            .uniform(ulocs_.projection, projection)
            .uniform(ulocs_.image, 0);

        vbo_.bind()
            .attach_data(quad.size(), quad.data(), GL_STATIC_DRAW)
            .associate_with<learn::Vertex2D>(vao_.bind());

    }


    void draw_sprites(entt::registry& registry) {

        using namespace gl;

        auto view = registry.view<Sprite, const Transform2D>();

        auto asp = sp_.use();
        auto bvao = vao_.bind();

        const learn::TextureHandle* last_texture{ nullptr };

        for (auto [ent, sprite, transform] : view.each()) {

            // Pointless optimizations are my forte...
            if (last_texture != sprite.texture.get()) {
                last_texture = sprite.texture.get();
                sprite.texture->bind_to_unit(GL_TEXTURE0);
            }

            asp .uniform(ulocs_.color, sprite.color)
                .uniform(ulocs_.model, transform.mtransform().translate({ 0.f, 0.f, sprite.depth }).model());

            // TODO(maybe): Instanced draws on sprites with the same texture?
            bvao.draw_arrays(GL_TRIANGLES, 0, quad.size());
        }

        bvao.unbind();
    }


};
