#pragma once
#include "MaterialConcept.hpp"
#include "GLObjects.hpp"
#include "Shared.hpp"

namespace learn {


struct MaterialDSLocations {
    gl::GLint diffuse{ -1 };
    gl::GLint specular{ -1 };
    gl::GLint shininess{ -1 };
};


/*
Diffuse-specular material for a classic workflow.

Requires shader uniforms:

sampler2D material.diffuse;
sampler2D material.specular;
float     material.shininess;

Implement as:

uniform struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
} material;

*/
struct MaterialDS {
    Shared<TextureHandle> diffuse;
    Shared<TextureHandle> specular;
    gl::GLfloat shininess{};

    using locations_type = MaterialDSLocations;

    void apply(ActiveShaderProgram& asp) const;
    void apply(ActiveShaderProgram& asp, const MaterialDSLocations& locs) const;
    static MaterialDSLocations query_locations(ActiveShaderProgram& asp);
    static MaterialDSLocations query_locations(ShaderProgram& sp);
};


static_assert(material<MaterialDS>);



} // namespace learn
