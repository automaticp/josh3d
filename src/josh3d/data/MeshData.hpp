#pragma once
#include <cstdint>
#include <vector>


namespace josh {


template<typename VertexT>
struct MeshData {
    using vertex_type  = VertexT;
    using element_type = uint32_t;

    std::vector<vertex_type>  vertices;
    std::vector<element_type> elements;
};


} // namespace josh
