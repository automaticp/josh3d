#pragma once
#include "GLObjects.hpp"
#include "MeshData.hpp"
#include "VertexTraits.hpp"

#include <glbinding/gl/gl.h>


namespace learn {


class Mesh {
private:
    VBO vbo_;
    VAO vao_;
    EBO ebo_;
    gl::GLsizei num_elements_;

public:
    template<typename V>
    Mesh(const MeshData<V>& data)
        : num_elements_{ static_cast<gl::GLsizei>(data.elements().size()) }
    {
        using namespace gl;

        // Ok, these 'and_then's are getting pretty ridiculous
        vao_.bind()
            .and_then_with_self([this, &data](BoundVAO& self) {

                vbo_.bind()
                    .attach_data(data.vertices().size(), data.vertices().data(), GL_STATIC_DRAW)
                    .associate_with(self, VertexTraits<V>::aparams);

                ebo_.bind(self)
                    .attach_data(num_elements_, data.elements().data(), GL_STATIC_DRAW);

            })
            .unbind();
    }

    void draw() {
        using namespace gl;

        vao_.bind()
           .draw_elements(GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT);
    }


    void draw_instanced(gl::GLsizei count) {
        using namespace gl;

        vao_.bind()
            .draw_elements_instanced(GL_TRIANGLES, num_elements_, GL_UNSIGNED_INT, count);
    }



};


} // namespace learn

