#pragma once
#include "Vertex.hpp"
#include <glbinding/gl/gl.h>
#include <vector>
#include <utility>

namespace learn {

template<typename V = Vertex>
class MeshData {
private:
    std::vector<V> vertices_;
    std::vector<gl::GLuint> elements_;

public:
    MeshData(std::vector<V> vertices, std::vector<gl::GLuint> elements)
        : vertices_{ std::move(vertices) }
        , elements_{ std::move(elements) }
    {}

    std::vector<V>& vertices() noexcept { return vertices_; }
    const std::vector<V>& vertices() const noexcept { return vertices_; }

    std::vector<gl::GLuint>& elements() noexcept { return elements_; }
    const std::vector<gl::GLuint>& elements() const noexcept { return elements_; }
};



} // namespace learn
