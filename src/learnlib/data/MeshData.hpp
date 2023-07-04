#pragma once
#include "GLScalars.hpp"
#include <glbinding/gl/gl.h>
#include <vector>
#include <utility>

namespace learn {

template<typename VertexT>
class MeshData {
private:
    std::vector<VertexT> vertices_;
    std::vector<GLuint> elements_;

public:
    MeshData(std::vector<VertexT> vertices, std::vector<GLuint> elements)
        : vertices_{ std::move(vertices) }
        , elements_{ std::move(elements) }
    {}

    std::vector<VertexT>& vertices() noexcept { return vertices_; }
    const std::vector<VertexT>& vertices() const noexcept { return vertices_; }

    std::vector<GLuint>& elements() noexcept { return elements_; }
    const std::vector<GLuint>& elements() const noexcept { return elements_; }
};



} // namespace learn
