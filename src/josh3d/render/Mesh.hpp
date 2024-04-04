#pragma once
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLAPICore.hpp"
#include "AttributeTraits.hpp" // IWYU pragma: keep (traits)
#include "GLBuffers.hpp"
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "GLVertexArray.hpp"
#include "MeshData.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>


namespace josh {


class Mesh {
private:
    UniqueUntypedBuffer  vbo_;
    UniqueBuffer<GLuint> ebo_;
    UniqueVertexArray    vao_;
    GLsizei num_elements_;
    GLsizei num_vertices_;

public:
    template<specializes_attribute_traits VertexT>
    Mesh(const MeshData<VertexT>& data);

    constexpr Primitive   primitive_type() const noexcept { return Primitive::Triangles; }
    constexpr ElementType element_type()   const noexcept { return ElementType::UInt;    }

    // TODO: Unindexed should not be an option. If so, it should be a separate type.
    bool is_indexed() const noexcept { return bool(num_elements_); }

    constexpr GLsizeiptr element_offset_bytes() const noexcept { return 0;             }
    GLsizei              num_elements()         const noexcept { return num_elements_; }
    constexpr GLsizei    vertex_offset()        const noexcept { return 0;             }
    GLsizei              num_vertices()         const noexcept { return num_vertices_; }

    RawVertexArray<GLConst> vertex_array() const noexcept { return vao_; }

    // TODO: This is an ass way of doing things. But anything better will need a rewrite.
    void draw(
        BindToken<Binding::Program>         bound_program,
        BindToken<Binding::DrawFramebuffer> bound_fbo) const
    {
        auto bound_vao = vertex_array().bind();
        if (is_indexed()) {
            glapi::draw_elements(
                bound_vao, bound_program, bound_fbo,
                primitive_type(), element_type(), element_offset_bytes(), num_elements()
            );
        } else {
            glapi::draw_arrays(
                bound_vao, bound_program, bound_fbo,
                primitive_type(), vertex_offset(), num_vertices()
            );
        }
    }

};




template<specializes_attribute_traits VertexT>
Mesh::Mesh(const MeshData<VertexT>& data)
    : num_elements_{ GLsizei(data.elements().size()) }
    , num_vertices_{ GLsizei(data.vertices().size()) }
{
    vbo_->as_typed<VertexT>().specify_storage(
        data.vertices(),
        StorageMode::StaticServer,
        PermittedMapping::NoMapping,
        PermittedPersistence::NotPersistent
    );

    vao_->attach_vertex_buffer(VertexBufferSlot{ 0 }, vbo_, OffsetBytes{ 0 }, StrideBytes{ sizeof(VertexT) });

    if (!data.elements().empty()) {
        ebo_->specify_storage(
            data.elements(),
            StorageMode::StaticServer,
            PermittedMapping::NoMapping,
            PermittedPersistence::NotPersistent
        );

        vao_->attach_element_buffer(ebo_);
    }


    const AttributeIndex first_attrib{ 0 };
    const GLsizeiptr num_attribs = vao_->specify_custom_attributes<VertexT>(first_attrib);

    for (GLuint attrib_id{ first_attrib }; attrib_id < num_attribs; ++attrib_id) {
        vao_->enable_attribute(AttributeIndex{ attrib_id });
        // All the vertex data goes through the 1st buffer slot.
        vao_->associate_attribute_with_buffer_slot(AttributeIndex{ attrib_id }, VertexBufferSlot{ 0 });
    }

}




} // namespace josh

