#pragma once
#include "MeshData.hpp"
#include "TextureData.hpp"
#include "VertexPNT.hpp"
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <memory>
#include <type_traits>


namespace josh {


template<typename MappingF>
concept height_mapping_fun =
    std::invocable<MappingF, unsigned char> &&
    std::same_as<std::invoke_result_t<MappingF, unsigned char>, float>;


class HeightmapData {
private:
    Size2S size_;
    std::unique_ptr<float[]> data_;

public:
    HeightmapData(Size2S image_size)
        : size_{ image_size }
        , data_{ std::make_unique_for_overwrite<float[]>(size_.area()) }
    {}

    // Generate a heightmap from the first channel of each pixel in `data`
    // by applying a linear mapping from [0-255] to [min_point-max_point].
    static HeightmapData from_texture(
        const TextureData& data, float min_point, float max_point);

    // Generate a heightmap from the first channel of each pixel in `data`
    // by applying a user-provided mapping.
    template<height_mapping_fun MappingF>
    static HeightmapData from_texture(
        const TextureData& data, MappingF&& mapping_fun);

    // size_t data_size() const noexcept { return size_.area() * n_channels(); }
    Size2S image_size() const noexcept { return size_; }
    size_t height() const noexcept { return size_.height; }
    size_t width() const noexcept { return size_.width; }
    // size_t n_channels() const noexcept { return 1; }
    size_t n_pixels() const noexcept { return size_.area(); }
          float* data()       noexcept { return data_.get(); }
    const float* data() const noexcept { return data_.get(); }

          float* begin()       noexcept { return data_.get(); }
    const float* begin() const noexcept { return data_.get(); }
          float* end()         noexcept { return begin() + n_pixels(); }
    const float* end()   const noexcept { return begin() + n_pixels(); }

          float& operator[](size_t idx)       noexcept { return data_[idx]; }
    const float& operator[](size_t idx) const noexcept { return data_[idx]; }

    // Row-major.
          float& at(size_t x, size_t y)       noexcept { return data_[x + y * width()]; }
    const float& at(size_t x, size_t y) const noexcept { return data_[x + y * width()]; }

private:
    HeightmapData(std::unique_ptr<float[]> data, Size2S image_size)
        : size_{ image_size }
        , data_{ std::move(data) }
    {}
};


template<height_mapping_fun MappingF>
inline HeightmapData HeightmapData::from_texture(
    const TextureData& data, MappingF&& mapping_fun)
{
    auto out = std::make_unique_for_overwrite<float[]>(data.n_pixels());

    for (size_t pid{ 0 }; pid < data.n_pixels(); ++pid) {
        // FIXME: This is clearly a compromise. Ideally, we shouldn't
        // even accept textures with n_channels != 1 as arguments.
        // But the number of channels is not encoded in the type...
        // Something to consider, really.
        size_t first_channel_id = pid * data.n_channels();
        out[pid] = std::forward<MappingF>(mapping_fun)(data[first_channel_id]);
    }

    return HeightmapData{ std::move(out), data.image_size() };
}


inline HeightmapData HeightmapData::from_texture(
    const TextureData& data, float min_point, float max_point)
{
    return from_texture(
        data,
        [&](unsigned char px) {
            return std::lerp(min_point, max_point, float(px));
        }
    );
}


template<of_signature<float(size_t, size_t)> MappingF>
[[nodiscard]] inline MeshData<VertexPNT> generate_terrain_mesh(
    Size2S num_vertices_xy, MappingF&& mapping_fun)
{
    const size_t size_x = num_vertices_xy.width;
    const size_t size_y = num_vertices_xy.height;

    assert(size_x > 1 && size_y > 1);

    MeshData<VertexPNT> result;
    result.vertices().reserve(num_vertices_xy.area());

    for (size_t yid{ 0 }; yid < size_y; ++yid) {
        for (size_t xid{ 0 }; xid < size_x; ++xid) {
            VertexPNT v{};

            v.tex_uv.s = float(xid) / float(size_x - 1);
            v.tex_uv.t = float(yid) / float(size_y - 1);

            v.position.x = v.tex_uv.s;
            v.position.y = std::invoke(std::forward<MappingF>(mapping_fun), xid, yid);
            v.position.z = v.tex_uv.t;

            // TODO: Sample adjacent pixels to get the local gradient?
            // TODO: Generate normals;
            v.normal = glm::vec3{ 0.f, 1.f, 0.f };

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
