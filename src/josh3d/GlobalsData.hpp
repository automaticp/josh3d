#pragma once
#include "Globals.hpp"
#include "DataPool.hpp"
#include "TextureData.hpp"
#include "MeshData.hpp"
#include "VertexPNTTB.hpp"


namespace josh::globals {

extern DataPool<TextureData> texture_data_pool;

const MeshData<VertexPNTTB>& plane_primitive() noexcept;
const MeshData<VertexPNTTB>& box_primitive() noexcept;
const MeshData<VertexPNTTB>& sphere_primitive() noexcept;

}
