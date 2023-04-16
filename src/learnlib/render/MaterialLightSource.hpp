#pragma once
#include "GLObjects.hpp"
#include "MaterialConcept.hpp"
#include "ULocation.hpp"
#include <glm/glm.hpp>
#include <glbinding/gl/types.h>



namespace learn {

/*
Simple single color material for drawing light sources.

Requires shader uniforms:

vec3 light_color;

Implement as:

uniform vec3 light_color;

*/
struct MaterialLightSource {
    glm::vec3 light_color;

    struct Locations {
        ULocation light_color;
    };
    using locations_type = Locations;

    void apply(ActiveShaderProgram& asp) const {
        apply(asp, query_locations(asp));
    }

    void apply(ActiveShaderProgram& asp, const Locations& locs) const {
        asp.uniform(locs.light_color, light_color);
    }

    static Locations query_locations(ActiveShaderProgram& asp) {
        return Locations{
            .light_color = asp.location_of("light_color")
        };
    }

    static Locations query_locations(ShaderProgram& sp) {
        return Locations{
            .light_color = sp.location_of("light_color")
        };
    }

};


static_assert(material<MaterialLightSource>);


} // namespace learn
