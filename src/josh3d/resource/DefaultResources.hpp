#pragma once
#include "GLMutability.hpp"
#include "GLTextures.hpp"
#include "Mesh.hpp"
#include "VertexPNTTB.hpp"
#include "MeshData.hpp"


namespace josh {


namespace globals {
dsa::RawTexture2D<GLConst> default_diffuse_texture()  noexcept;
dsa::RawTexture2D<GLConst> default_specular_texture() noexcept;
dsa::RawTexture2D<GLConst> default_normal_texture()   noexcept;
dsa::RawCubemap<GLConst>   debug_skybox_cubemap()     noexcept;

const MeshData<VertexPNTTB>& plane_primitive_data()  noexcept;
const MeshData<VertexPNTTB>& box_primitive_data()    noexcept;
const MeshData<VertexPNTTB>& sphere_primitive_data() noexcept;

const Mesh& plane_primitive_mesh()  noexcept;
const Mesh& box_primitive_mesh()    noexcept;
const Mesh& sphere_primitive_mesh() noexcept;
const Mesh& quad_primitive_mesh()   noexcept;
} // namespace globals


namespace detail {
void init_default_textures();
void reset_default_textures();
void init_mesh_primitives();
void reset_mesh_primitives();
} // namespace detail


} // namespace josh
