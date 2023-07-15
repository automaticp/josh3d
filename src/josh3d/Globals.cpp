#include "Globals.hpp"
#include "DataPool.hpp"
#include "FrameTimer.hpp"
#include "GLObjectPool.hpp"
#include "TextureHandlePool.hpp"
#include "GLObjects.hpp"
#include "TextureData.hpp"
#include "MeshData.hpp"
#include "VertexPNTTB.hpp"
#include "AssimpModelLoader.hpp"
#include "Shared.hpp"
#include "WindowSizeCache.hpp"
#include <assimp/postprocess.h>
#include <glbinding/gl/enum.h>
#include <iostream>
#include <array>
#include <memory>



namespace josh::globals {

DataPool<TextureData> texture_data_pool;
TextureHandlePool texture_handle_pool{ texture_data_pool };

std::ostream& logstream{ std::clog };

Shared<Texture2D> default_diffuse_texture;
Shared<Texture2D> default_specular_texture;
Shared<Texture2D> default_normal_texture;


static MeshData<VertexPNTTB> plane_primitive_;
static MeshData<VertexPNTTB> box_primitive_;
static MeshData<VertexPNTTB> sphere_primitive_;


const MeshData<VertexPNTTB>& plane_primitive() noexcept { return plane_primitive_; }
const MeshData<VertexPNTTB>& box_primitive() noexcept { return box_primitive_;  }
const MeshData<VertexPNTTB>& sphere_primitive() noexcept{ return sphere_primitive_; };




static TextureData fill_default(std::array<unsigned char, 4> rgba) {
    TextureData img{ 1, 1, 4 };
    const size_t n_channels = img.n_channels();
    for (size_t i{ 0 }; i < img.n_pixels(); ++i) {
        const size_t idx = i * n_channels;
        img[idx + 0] = rgba[0];
        img[idx + 1] = rgba[1];
        img[idx + 2] = rgba[2];
        img[idx + 3] = rgba[3];
    }
    return img;
}


static Shared<Texture2D> init_default_diffuse_texture() {
    auto tex = fill_default({ 0xB0, 0xB0, 0xB0, 0xFF });
    auto handle = std::make_shared<Texture2D>();
    handle->bind().attach_data(tex, gl::GL_SRGB_ALPHA).unbind();
    return handle;
}

static Shared<Texture2D> init_default_specular_texture() {
    auto tex = fill_default({ 0x00, 0x00, 0x00, 0xFF });
    auto handle = std::make_shared<Texture2D>();
    handle->bind().attach_data(tex, gl::GL_RGBA).unbind();
    return handle;
}

static Shared<Texture2D> init_default_normal_texture() {
    auto tex = fill_default({ 0x7F, 0x7F, 0xFF, 0xFF });
    auto handle = std::make_shared<Texture2D>();
    handle->bind().attach_data(tex, gl::GL_RGBA).unbind();
    return handle;
}


void init_all() {
    default_diffuse_texture  = init_default_diffuse_texture();
    default_specular_texture = init_default_specular_texture();
    default_normal_texture   = init_default_normal_texture();

    AssimpMeshDataLoader<VertexPNTTB> loader;
    loader.add_flags(aiProcess_CalcTangentSpace);

    box_primitive_    = loader.load("data/primitives/box.obj").get()[0];
    plane_primitive_  = loader.load("data/primitives/plane.obj").get()[0];
    sphere_primitive_ = loader.load("data/primitives/sphere.obj").get()[0];
}


void clear_all() {
    texture_data_pool.clear();
    texture_handle_pool.clear();
    default_diffuse_texture.reset();
    default_specular_texture.reset();
    default_normal_texture.reset();
}


} // namespace josh::globals
