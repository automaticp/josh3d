#pragma once
#include "CommonConcepts.hpp"
#include "AttributeParams.hpp"


namespace josh {



template<typename AttrsT>
concept vertex_attribute_container =
    requires(const AttrsT& aparams) {
        {
            [](const AttrsT& aparams) {
                for (const AttributeParams& attr : aparams) {
                    return attr;
                }
            }(aparams)
        } -> same_as_remove_cvref<AttributeParams>;
    };



template<typename VertexT>
concept vertex =
    requires {

        requires vertex_attribute_container<typename VertexT::attributes_type>;

        { VertexT::get_attributes() }
            -> same_as_remove_cvref<typename VertexT::attributes_type>;

    };





} // namespace josh
