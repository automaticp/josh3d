#pragma once
#include "MaterialConcept.hpp"
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "Shared.hpp"
#include "ULocation.hpp"



namespace josh {


/*
Diffuse-specular-normal material for a classic workflow.

Requires shader uniforms:

sampler2D material.diffuse;
sampler2D material.specular;
sampler2d material.normal;
float     material.shininess;

Implement as:

uniform struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
    float shininess;
} material;

*/
struct MaterialDSN {
    Shared<Texture2D> diffuse;
    Shared<Texture2D> specular;
    Shared<Texture2D> normal;
    GLfloat shininess{};


    struct Locations {
        ULocation diffuse;
        ULocation specular;
        ULocation normal;
        ULocation shininess;
    };

    using locations_type = MaterialDSN::Locations;


    void apply(ActiveShaderProgram& asp) const;
    void apply(ActiveShaderProgram& asp, const locations_type& locs) const;
    static locations_type query_locations(ActiveShaderProgram& asp);
    static locations_type query_locations(ShaderProgram& sp);
};


static_assert(material<MaterialDSN>);




} // namespace josh


