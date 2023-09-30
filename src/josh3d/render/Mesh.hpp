#pragma once
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "MeshData.hpp"
#include "VertexConcept.hpp"
#include <glbinding/gl/gl.h>


namespace josh {


class Mesh {
private:
    UniqueVBO vbo_;
    UniqueVAO vao_;
    UniqueEBO ebo_;
    GLsizei num_elements_;

public:
    template<vertex V>
    Mesh(const MeshData<V>& data)
        : num_elements_{ static_cast<gl::GLsizei>(data.elements().size()) }
    {
        using enum GLenum;

        // Ok, these 'and_then's are getting pretty ridiculous
        vao_.bind()
            .and_then([&, this](BoundVAO<GLMutable>& bvao) {

                vbo_.bind()
                    .specify_data(data.vertices().size(), data.vertices().data(), GL_STATIC_DRAW)
                    .template associate_with<V>(bvao);

                ebo_.bind()
                    .specify_data(num_elements_, data.elements().data(), GL_STATIC_DRAW);

            })
            .unbind();
    }

    void draw() const {
        using enum GLenum;

        vao_.bind()
            .draw_elements(GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT);
    }


    void draw_instanced(GLsizei count) const {
        using enum GLenum;

        vao_.bind()
            .draw_elements_instanced(GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT, count);
    }



};


} // namespace josh

