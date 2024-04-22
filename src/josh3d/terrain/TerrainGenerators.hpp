#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)
#include "MeshData.hpp"
#include "Size.hpp"
#include "Index.hpp"
#include "VertexPNUTB.hpp"
#include <cassert>
#include <glm/glm.hpp>


namespace josh {


template<of_signature<float(const Index2S&)> MappingF>
[[nodiscard]] inline auto generate_terrain_mesh(
    Size2S     num_vertices_xy,
    MappingF&& mapping_fun)
        -> MeshData<VertexPNUTB>
{
    const size_t size_x = num_vertices_xy.width;
    const size_t size_y = num_vertices_xy.height;

    assert(size_x > 1 && size_y > 1);

    MeshData<VertexPNUTB> result;
    result.vertices().reserve(num_vertices_xy.area());

    for (size_t yid{ 0 }; yid < size_y; ++yid) {
        for (size_t xid{ 0 }; xid < size_x; ++xid) {
            VertexPNUTB v{};

            v.uv.s = float(xid) / float(size_x - 1);
            v.uv.t = float(yid) / float(size_y - 1);

            v.position.x = v.uv.s;
            v.position.y = std::invoke(std::forward<MappingF>(mapping_fun), Index2S{ xid, yid });
            v.position.z = v.uv.t;

            // Replaced later in a second pass.
            v.normal = glm::vec3{ 0.f, 1.f, 0.f };

            // Tangents are ignored for now.
            v.tangent   = {};
            v.bitangent = {};

            result.vertices().emplace_back(v);
        }
    }

    result.elements().reserve(6 * (size_x - 1) * (size_y - 1));
    unsigned quad_id{ 0 };
    for (size_t yid{ 0 }; yid < size_y - 1; ++yid) {
        for (size_t xid{ 0 }; xid < size_x - 1; ++xid) {
            // Describe quads as pairs of tris. Split each quad as "\".
            struct QuadIds { unsigned tl, tr, bl, br; };
            QuadIds quad{
                .tl = quad_id,
                .tr = quad_id + 1,
                .bl = quad_id + unsigned(size_x),
                .br = quad_id + unsigned(size_x) + 1
            };

            auto emplace_triangle = [&](unsigned v0, unsigned v1, unsigned v2) {
                result.elements().emplace_back(v0);
                result.elements().emplace_back(v1);
                result.elements().emplace_back(v2);
            };

            // Winding order is CCW.

            // Triangle |\:
            emplace_triangle(quad.tl, quad.bl, quad.br);
            // Triangle \|:
            emplace_triangle(quad.tl, quad.br, quad.tr);

            ++quad_id;
        }
        ++quad_id; // Skip the last vertex each row.
    }


    for (size_t tri_id{ 0 }; tri_id < result.elements().size(); tri_id += 3) {
        unsigned v0id = result.elements()[tri_id];
        unsigned v1id = result.elements()[tri_id + 1];
        unsigned v2id = result.elements()[tri_id + 2];

        auto& v0 = result.vertices()[v0id];
        auto& v1 = result.vertices()[v1id];
        auto& v2 = result.vertices()[v2id];

        glm::vec3 normal = glm::normalize(
            glm::cross(v1.position - v0.position, v2.position - v0.position)
        );

        // Flat, but?

        v0.normal = normal;
        v1.normal = normal;
        v2.normal = normal;
    }


    return result;
}


} // namespace josh
