#pragma once
#include "GLObjects.hpp"
#include "ShaderBuilder.hpp"
#include "Transform.hpp"
#include "Vertex2D.hpp"
#include "VPath.hpp"

namespace josh {


class SpriteRenderer {
private:
    VBO vbo_;
    VAO vao_;
    ShaderProgram sp_;

    constexpr static std::array<Vertex2D, 6> quad{ {
        { { -0.5f, +0.5f }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f }, { 0.0f, 0.0f } },
        { { +0.5f, -0.5f }, { 1.0f, 0.0f } },

        { { +0.5f, +0.5f }, { 1.0f, 1.0f } },
        { { -0.5f, +0.5f }, { 0.0f, 1.0f } },
        { { +0.5f, -0.5f }, { 1.0f, 0.0f } },
    } };

public:
    SpriteRenderer(const glm::mat4& projection)
        : sp_{
            ShaderBuilder()
                .load_vert(VPath("src/shaders/sprite.vert"))
                .load_frag(VPath("src/shaders/sprite.frag"))
                .get()
        }
    {
        using namespace gl;

        sp_.use().uniform("projection", projection);

        vbo_.bind()
            .attach_data(quad.size(), quad.data(), GL_STATIC_DRAW)
            .associate_with<Vertex2D>(vao_.bind());

    }



    void draw_sprite(Texture2D& texture, const MTransform& transform,
        const glm::vec4& color = glm::vec4{ 1.f })
    {
        using namespace gl;

        // FIXME: State change on every draw.
        // Give option to draw a range?

        // FIXME: Can have an instanced version for sprites
        // with the same texture.

        auto asp = sp_.use();

        texture.bind_to_unit(GL_TEXTURE0);

        asp .uniform("model", transform.model())
            .uniform("color", color)
            .uniform("image", 0);

        vao_.bind()
            .draw_arrays(GL_TRIANGLES, 0, quad.size())
            .unbind();
    }


};




} // namespace josh
