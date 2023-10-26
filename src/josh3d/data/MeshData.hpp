#pragma once
#include <vector>
#include <utility>

namespace josh {


template<typename VertexT>
class MeshData {
private:
    std::vector<VertexT> vertices_;
    std::vector<unsigned> elements_;

public:
    MeshData() = default;

    MeshData(std::vector<VertexT> vertices, std::vector<unsigned> elements)
        : vertices_{ std::move(vertices) }
        , elements_{ std::move(elements) }
    {}

    std::vector<VertexT>& vertices() noexcept { return vertices_; }
    const std::vector<VertexT>& vertices() const noexcept { return vertices_; }

    std::vector<unsigned>& elements() noexcept { return elements_; }
    const std::vector<unsigned>& elements() const noexcept { return elements_; }
};


} // namespace josh
