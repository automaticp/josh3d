#pragma once
#include "GLMutability.hpp"
#include "GLTextures.hpp"
#include "VertexPNTTB.hpp"
#include "MeshData.hpp"

namespace josh {


namespace globals {
RawTexture2D<GLConst> default_diffuse_texture()  noexcept;
RawTexture2D<GLConst> default_specular_texture() noexcept;
RawTexture2D<GLConst> default_normal_texture()   noexcept;

const MeshData<VertexPNTTB>& plane_primitive_data()  noexcept;
const MeshData<VertexPNTTB>& box_primitive_data()    noexcept;
const MeshData<VertexPNTTB>& sphere_primitive_data() noexcept;
} // namespace globals


namespace detail {
void init_default_textures();
void reset_default_textures();
void init_mesh_primitives();
} // namespace detail


} // namespace josh
