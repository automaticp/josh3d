#include "components/TerrainChunk.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "PixelData.hpp"
#include "Pixels.hpp"
#include "PixelPackTraits.hpp" // IWYU pragma: keep
#include "Region.hpp"
#include "VertexFormats.hpp"
#include <random>
#include <utility>


namespace josh {
namespace {

auto generate_terrain_heightmap_data(float max_height, const Size2S& resolution)
    -> PixelData<pixel::RedF>
{
    thread_local std::mt19937 urng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist{ 0.f, max_height };

    PixelData<pixel::RedF> heightmap_data{ resolution };
    for (auto& px : heightmap_data)
        px.r = dist(urng);

    return heightmap_data;
}

template<of_signature<float(const Index2S&)> MappingF>
[[nodiscard]] inline auto generate_terrain_mesh(
    const Extent2F& extents,
    const Size2S&   num_vertices_xy,
    MappingF&&      mapping_fun)
        -> MeshData<VertexStatic>
{
    const size_t size_x = num_vertices_xy.width;
    const size_t size_y = num_vertices_xy.height;

    assert(size_x > 1 && size_y > 1);

    MeshData<VertexStatic> result;
    result.vertices.reserve(num_vertices_xy.area());

    for (size_t yid{ 0 }; yid < size_y; ++yid)
    {
        for (size_t xid{ 0 }; xid < size_x; ++xid)
        {
            VertexStatic v = {};

            v.uv.s = float(xid) / float(size_x - 1);
            v.uv.t = float(yid) / float(size_y - 1);

            v.position.x = v.uv.s * extents.width;
            v.position.y = mapping_fun(Index2S(xid, yid));
            v.position.z = v.uv.t * extents.height;

            // Replaced later in a second pass.
            v.normal = glm::vec3{ 0.f, 1.f, 0.f };

            // Tangents are ignored for now.
            v.tangent = {};

            result.vertices.emplace_back(v);
        }
    }

    result.elements.reserve(6 * (size_x - 1) * (size_y - 1));
    unsigned quad_id{ 0 };
    for (size_t yid{ 0 }; yid < size_y - 1; ++yid)
    {
        for (size_t xid{ 0 }; xid < size_x - 1; ++xid)
        {
            // Describe quads as pairs of tris. Split each quad as "\".
            struct QuadIds { unsigned tl, tr, bl, br; };
            QuadIds quad{
                .tl = quad_id,
                .tr = quad_id + 1,
                .bl = quad_id + unsigned(size_x),
                .br = quad_id + unsigned(size_x) + 1
            };

            auto emplace_triangle = [&](unsigned v0, unsigned v1, unsigned v2) {
                result.elements.emplace_back(v0);
                result.elements.emplace_back(v1);
                result.elements.emplace_back(v2);
            };

            // Winding order is CCW.

            emplace_triangle(quad.tl, quad.bl, quad.br); // |\ triangle.
            emplace_triangle(quad.tl, quad.br, quad.tr); // \| triangle.
            ++quad_id;
        }
        ++quad_id; // Skip the last vertex each row.
    }


    for (size_t tri_id{ 0 }; tri_id < result.elements.size(); tri_id += 3)
    {
        unsigned v0id = result.elements[tri_id];
        unsigned v1id = result.elements[tri_id + 1];
        unsigned v2id = result.elements[tri_id + 2];

        auto& v0 = result.vertices[v0id];
        auto& v1 = result.vertices[v1id];
        auto& v2 = result.vertices[v2id];

        glm::vec3 normal = glm::normalize(
            glm::cross(v1.position - v0.position, v2.position - v0.position));

        // Flat, but?

        v0.normal = normal;
        v1.normal = normal;
        v2.normal = normal;
    }


    return result;
}

auto create_heightmap_texture(const PixelData<pixel::RedF>& heightmap)
    -> UniqueTexture2D
{
    Size2I resolution{ heightmap.resolution() };
    UniqueTexture2D texture;
    texture->allocate_storage(resolution, InternalFormat::R32F, max_num_levels(resolution));
    texture->upload_image_region({ {}, resolution }, heightmap.data());
    texture->generate_mipmaps();
    texture->set_sampler_min_mag_filters(MinFilter::NearestMipmapLinear, MagFilter::Nearest);
    return texture;
}

} // namespace


auto create_terrain_chunk(
    float           max_height,
    const Extent2F& extents,
    const Size2S&   resolution)
        -> TerrainChunk
{
    auto heightmap_data = generate_terrain_heightmap_data(max_height, resolution);
    auto mesh_data      = generate_terrain_mesh(extents, resolution, [&](const Index2S& idx) { return heightmap_data.at(idx).r; });
    auto mesh           = Mesh(mesh_data);
    auto heightmap      = create_heightmap_texture(heightmap_data);
    return {
        .mesh           = std::move(mesh),
        .heightmap_data = std::move(heightmap_data),
        .heightmap      = std::move(heightmap),
    };
}


} // namespace josh
