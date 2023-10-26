#pragma once
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "AttributeParams.hpp" // IWYU pragma: keep (concepts)
#include "MeshData.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>


namespace josh {


class Mesh {
private:
    UniqueVBO vbo_;
    UniqueEBO ebo_;
    UniqueVAO vao_;
    GLsizei num_elements_;
    GLsizei num_vertices_;

public:
    template<vertex V>
    Mesh(const MeshData<V>& data)
        : num_elements_{ GLsizei(data.elements().size()) }
        , num_vertices_{ GLsizei(data.vertices().size()) }
    {
        using enum GLenum;

        // Ok, these 'and_then's are getting pretty ridiculous
        vao_.bind()
            .and_then([&, this](BoundVAO<GLMutable>& bvao) {

                vbo_.bind()
                    .specify_data<V>(data.vertices(), GL_STATIC_DRAW)
                    .template associate_with<V>(bvao);

                if (is_indexed()) {
                    ebo_.bind()
                       .specify_data<GLuint>(data.elements(), GL_STATIC_DRAW);
                }

            })
            .unbind();
    }

    void draw() const {
        using enum GLenum;

        if (is_indexed()) {
            vao_.bind().draw_elements(GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT);
        } else {
            vao_.bind().draw_arrays(GL_TRIANGLES, 0, num_vertices_);
        }
    }

    void draw_instanced(GLsizei instance_count) const {
        using enum GLenum;

        if (is_indexed()) {
            vao_.bind()
                .draw_elements_instanced(GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT, instance_count);
        } else {
            vao_.bind()
                .draw_arrays_instanced(GL_TRIANGLES, 0, num_vertices_, instance_count);
        }
    }

    bool is_indexed() const noexcept { return bool(num_elements_); }

};


} // namespace josh

