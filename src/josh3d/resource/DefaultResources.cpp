#include "DefaultResources.hpp"
#include "CubemapData.hpp"
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "ImageData.hpp"
#include "Pixels.hpp"
#include "TextureHelpers.hpp"
#include "MeshData.hpp"
#include "Mesh.hpp"
#include "AssimpModelLoader.hpp"
#include "VPath.hpp"
#include "Vertex2D.hpp"
#include <glbinding/gl/enum.h>
#include <optional>


namespace josh {


namespace {


ImageData<pixel::RGBA> make_filled_image_data(pixel::RGBA rgba, const Size2S& size) {
    ImageData<pixel::RGBA> img{ size };
    for (auto& px : img) { px = rgba; }
    return img;
}


UniqueTexture2D create_filled(
    pixel::RGBA rgba, const Size2S& size,
    GLenum internal_format)
{
    auto tex_data = make_filled_image_data(rgba, size);
    UniqueTexture2D handle;
    handle.bind()
        .and_then([&](BoundTexture2D<GLMutable>& btex) {
            attach_data_to_texture(btex, tex_data, internal_format);
        })
        .generate_mipmaps()
        .unbind();
    return handle;
}


std::optional<UniqueTexture2D> default_diffuse_texture_;
std::optional<UniqueTexture2D> default_specular_texture_;
std::optional<UniqueTexture2D> default_normal_texture_;
std::optional<UniqueCubemap>   debug_skybox_cubemap_;

MeshData<VertexPNTTB> plane_primitive_data_;
MeshData<VertexPNTTB> box_primitive_data_;
MeshData<VertexPNTTB> sphere_primitive_data_;
MeshData<Vertex2D>    quad_primitive_data_{{
    { { +1.0f, -1.0f }, { 1.0f, 0.0f } },
    { { -1.0f, +1.0f }, { 0.0f, 1.0f } },
    { { -1.0f, -1.0f }, { 0.0f, 0.0f } },
    { { +1.0f, +1.0f }, { 1.0f, 1.0f } },
    { { -1.0f, +1.0f }, { 0.0f, 1.0f } },
    { { +1.0f, -1.0f }, { 1.0f, 0.0f } },
}};

std::optional<Mesh> plane_primitive_mesh_;
std::optional<Mesh> box_primitive_mesh_;
std::optional<Mesh> sphere_primitive_mesh_;
std::optional<Mesh> quad_primitive_mesh_;


} // namespace


namespace globals {
// NOLINTBEGIN(bugprone-unchecked-optional-access): Will terminate on bad access anyway.
RawTexture2D<GLConst> default_diffuse_texture()  noexcept { return default_diffuse_texture_ .value(); }
RawTexture2D<GLConst> default_specular_texture() noexcept { return default_specular_texture_.value(); }
RawTexture2D<GLConst> default_normal_texture()   noexcept { return default_normal_texture_  .value(); }
RawCubemap<GLConst>   debug_skybox_cubemap()     noexcept { return debug_skybox_cubemap_    .value(); }

const MeshData<VertexPNTTB>& plane_primitive_data()  noexcept { return plane_primitive_data_;  }
const MeshData<VertexPNTTB>& box_primitive_data()    noexcept { return box_primitive_data_;    }
const MeshData<VertexPNTTB>& sphere_primitive_data() noexcept { return sphere_primitive_data_; }

const Mesh& plane_primitive_mesh()  noexcept { return plane_primitive_mesh_ .value(); }
const Mesh& box_primitive_mesh()    noexcept { return box_primitive_mesh_   .value(); }
const Mesh& sphere_primitive_mesh() noexcept { return sphere_primitive_mesh_.value(); }
const Mesh& quad_primitive_mesh()   noexcept { return quad_primitive_mesh_  .value(); }
// NOLINTEND(bugprone-unchecked-optional-access)
} // namespace globals




void detail::init_default_textures() {
    using enum GLenum;
    default_diffuse_texture_  = create_filled({ 0xB0, 0xB0, 0xB0, 0xFF }, {1, 1}, GL_SRGB_ALPHA);
    default_specular_texture_ = create_filled({ 0x00, 0x00, 0x00, 0xFF }, {1, 1}, GL_RGBA);
    default_normal_texture_   = create_filled({ 0x7F, 0x7F, 0xFF, 0xFF }, {1, 1}, GL_RGBA);
    debug_skybox_cubemap_     = [] {
        CubemapData data{
            load_cubemap_from_json<pixel::RGBA>(VPath("data/skyboxes/debug/skybox.json"))
        };
        UniqueCubemap cube;
        cube.bind()
            .and_then([&data](BoundCubemap<GLMutable>& cube) {
                attach_data_to_cubemap(cube, data, TexSpec{ GL_SRGB_ALPHA });
            })
            .set_min_mag_filters(GL_NEAREST, GL_NEAREST)
            .set_wrap_st(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE)
            .unbind();

        return cube;
    }();
}


void detail::reset_default_textures() {
    default_diffuse_texture_ .reset();
    default_specular_texture_.reset();
    default_normal_texture_  .reset();
    debug_skybox_cubemap_    .reset();
}


void detail::init_mesh_primitives() {

    AssimpMeshDataLoader<VertexPNTTB> loader;
    loader.add_flags(aiProcess_CalcTangentSpace);

    box_primitive_data_    = loader.load(VPath("data/primitives/box.obj")).get()[0];
    plane_primitive_data_  = loader.load(VPath("data/primitives/plane.obj")).get()[0];
    sphere_primitive_data_ = loader.load(VPath("data/primitives/sphere.obj")).get()[0];

    box_primitive_mesh_    = Mesh{ box_primitive_data_    };
    plane_primitive_mesh_  = Mesh{ plane_primitive_data_  };
    sphere_primitive_mesh_ = Mesh{ sphere_primitive_data_ };
    quad_primitive_mesh_   = Mesh{ quad_primitive_data_   };
}


void detail::reset_mesh_primitives() {
    plane_primitive_mesh_ .reset();
    box_primitive_mesh_   .reset();
    sphere_primitive_mesh_.reset();
    quad_primitive_mesh_  .reset();
}

} // namespace josh
