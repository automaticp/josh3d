#include "MeshData.hpp"
#include "VertexPNTTB.hpp"
#include "AssimpModelLoader.hpp"
#include "VPath.hpp"


namespace josh {


static MeshData<VertexPNTTB> plane_primitive_;
static MeshData<VertexPNTTB> box_primitive_;
static MeshData<VertexPNTTB> sphere_primitive_;


namespace globals {
const MeshData<VertexPNTTB>& plane_primitive()  noexcept { return plane_primitive_; }
const MeshData<VertexPNTTB>& box_primitive()    noexcept { return box_primitive_;  }
const MeshData<VertexPNTTB>& sphere_primitive() noexcept { return sphere_primitive_; };
} // namespace globals


void detail::init_mesh_primitives() {

    AssimpMeshDataLoader<VertexPNTTB> loader;
    loader.add_flags(aiProcess_CalcTangentSpace);

    box_primitive_    = loader.load(VPath("data/primitives/box.obj")).get()[0];
    plane_primitive_  = loader.load(VPath("data/primitives/plane.obj")).get()[0];
    sphere_primitive_ = loader.load(VPath("data/primitives/sphere.obj")).get()[0];

}


} // namespace josh
