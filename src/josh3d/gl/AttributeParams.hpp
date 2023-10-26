#pragma once
#include "GLScalars.hpp"
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)


namespace josh {


/*
Attribute specification pack for attaching
VBOs to Vertex Arrays.
*/
struct AttributeParams {
    GLuint      index;
    GLint       size;
    GLenum      type;
    GLboolean   normalized;
    GLsizei     stride_bytes;
    GLint64     offset_bytes;
};


template<typename VertexT> struct attribute_traits;


template<typename AttrsT>
concept vertex_attribute_container = requires(const AttrsT& aparams) {
    {
        // How illegal is this?
        [](const AttrsT& aparams) {
            for (const AttributeParams& attr : aparams) {
                return attr;
            }
        }(aparams)
    } -> same_as_remove_cvref<AttributeParams>;
};


template<typename VertexT>
concept vertex = requires {
    sizeof(attribute_traits<VertexT>);
    requires vertex_attribute_container<typename attribute_traits<VertexT>::params_type>;
    { attribute_traits<VertexT>::get_params() }
        -> same_as_remove_cvref<typename attribute_traits<VertexT>::params_type>;

};


template<> struct attribute_traits<struct Vertex2D> {
    using params_type = AttributeParams[2];
    static const params_type& get_params() noexcept { return aparams; }
    static const params_type aparams;
};

template<> struct attribute_traits<struct VertexPNT> {
    using params_type = AttributeParams[3];
    static const params_type& get_params() noexcept { return aparams; }
    static const params_type aparams;
};

template<> struct attribute_traits<struct VertexPNTTB> {
    using params_type = AttributeParams[5];
    static const params_type& get_params() noexcept { return aparams; }
    static const params_type aparams;
};


} // namespace josh
