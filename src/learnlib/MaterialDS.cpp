#include "GLObjects.hpp"
#include "MaterialDS.hpp"
#include <glbinding/gl/gl.h>


namespace learn {



void MaterialDS::apply(ActiveShaderProgram& asp) const {
    apply(asp, query_locations(asp));
}

void MaterialDS::apply(ActiveShaderProgram& asp, const MaterialDSLocations& locations) const {
    using namespace gl;
    diffuse->bind_to_unit(GL_TEXTURE0);
    asp.uniform(locations.diffuse, 0);
    specular->bind_to_unit(GL_TEXTURE1);
    asp.uniform(locations.specular, 1);
    asp.uniform(locations.shininess, this->shininess);
}

MaterialDSLocations MaterialDS::query_locations(ActiveShaderProgram& asp) {
    return MaterialDSLocations{
        .diffuse = asp.location_of("material.diffuse"),
        .specular = asp.location_of("material.specular"),
        .shininess = asp.location_of("material.shininess")
    };
}

MaterialDSLocations MaterialDS::query_locations(ShaderProgram& sp) {
    return MaterialDSLocations{
        .diffuse = sp.location_of("material.diffuse"),
        .specular = sp.location_of("material.specular"),
        .shininess = sp.location_of("material.shininess")
    };
}



} // namespace learn

