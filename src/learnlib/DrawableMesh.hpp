#pragma once
#include "MaterialDS.hpp"
#include "GLObjects.hpp"
#include "Mesh.hpp"
#include <utility>


namespace learn {


// A composition between a mesh and a material.
// A subtle suggestion that these components are independent
// and that MAYBE ACTUALLY SEPARATE THEM HELLO.
class DrawableMesh {
private:
    Mesh mesh_;
    MaterialDS material_;

public:
    DrawableMesh(Mesh mesh, MaterialDS material)
        : mesh_{ std::move(mesh) }
        , material_{ std::move(material) }
    {}

    Mesh& mesh() noexcept { return mesh_; }
    const Mesh& mesh() const noexcept { return mesh_; }

    MaterialDS& material() noexcept { return material_; }
    const MaterialDS& material() const noexcept { return material_; }


    void draw(ActiveShaderProgram& asp) {
        apply_material(asp, material_);
        mesh_.draw();
    }

    void draw(ActiveShaderProgram& asp, const MaterialDSLocations& locations) {
        apply_material(asp, material_, locations);
        mesh_.draw();
    }

    void draw_instanced(ActiveShaderProgram& asp, gl::GLsizei count) {
        apply_material(asp, material_);
        mesh_.draw_instanced(count);
    }

    void draw_instanced(ActiveShaderProgram& asp,
        const MaterialDSLocations& locations, gl::GLsizei count)
    {
        apply_material(asp, material_, locations);
        mesh_.draw_instanced(count);
    }

};



} // namespace learn
