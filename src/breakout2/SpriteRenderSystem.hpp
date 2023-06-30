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
    static constexpr float near{ +0.0f };
    static constexpr float far { +1.0f };
    static constexpr float background{ +0.8f };
    static constexpr float foreground{ +0.2f };
};

struct Sprite {
    learn::Shared<learn::Texture2D> texture;
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
                .load_vert("src/breakout/shaders/sprite.vert")
                .load_frag("src/breakout/shaders/sprite.frag")
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

        glEnable(GL_DEPTH_TEST);

        const learn::Texture2D* last_texture{ nullptr };

        for (auto [ent, sprite, transform] : view.each()) {

            // Pointless optimizations are my forte...
            if (last_texture != sprite.texture.get()) {
                last_texture = sprite.texture.get();
                sprite.texture->bind_to_unit(GL_TEXTURE0);
            }

            asp.uniform(ulocs_.color, sprite.color);

            // See below for an explanation of '-sprite.depth'.
            asp.uniform(ulocs_.model, transform.mtransform().translate({ 0.f, 0.f, -sprite.depth }).model());

            // TODO(maybe): Instanced draws on sprites with the same texture?
            bvao.draw_arrays(GL_TRIANGLES, 0, quad.size());
        }

        bvao.unbind();
    }


};

/*
We negate the sprite.depth when applying the Z transform for depth testing because
our chosen coordinate system (X, Y, Z=depth) with the origin at the bottom left
of the screen forms a Left-Handed (LH) coordinate system:

Y
|   Z
|  /
| /
|/______ X

where the larger values of Z (depth) represent objects further away (background).

There are 2 things that you have to keep in mind:

1. OpenGL (and GLM) expect the objects to be positioned in a Right-Handed (RH)
coordinate system.

2. The Normalized Device Coordinates (NDC) of clip space are actually using
a Left-Handed coordinate system.

What this means is that GLM, when forming a projection matrix with the
glm::ortho call, will encode the transformation from a RH to a LH system
in the projection matrix. You can see this by inspecting the matrix:
element P[3,3] (1-indexed), which represents linear scaling of the Z coordinate
will most likely be negative.

Because, once again, GLM expects a RH system, we just invert the Z component
to go from the LH to the RH coordinate system.
*/
