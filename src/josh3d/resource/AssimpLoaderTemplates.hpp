#pragma once
#include "GLScalars.hpp"
#include <assimp/mesh.h>
#include <vector>
#include <span>


namespace josh {


std::vector<GLuint> get_element_data(const aiMesh* mesh);


// Provide specialization for your own Vertex layout.
template<typename VertexT>
std::vector<VertexT> get_vertex_data(const aiMesh* mesh);


} // namespace josh
