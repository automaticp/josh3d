#include "MaterialTraits.hpp"
#include "MaterialDS.hpp"


namespace learn {


template<>
void apply_material<MaterialDS>(ActiveShaderProgram& asp, const MaterialDS& mat,
    const MaterialDSLocations& locations)
{
    size_t n_textures{ std::size(mat.textures) };
    for (size_t i{ 0 }; i < n_textures; ++i) {
        const MaterialParams& mp = MaterialTraits<MaterialDS>::texparams[i];
        mat.textures[i]->bind_to_unit(mp.tex_unit);
        asp.uniform(locations.textures[i], mp.sampler_uniform());
    }
    asp.uniform(locations.shininess, mat.shininess);
}


template<>
void apply_material<MaterialDS>(ActiveShaderProgram& asp, const MaterialDS& mat) {
    const auto& tps = MaterialTraits<MaterialDS>::texparams;
    apply_material(
        asp, mat,
        MaterialDSLocations{
            { asp.location_of(tps[0].name), asp.location_of(tps[1].name) },
            asp.location_of("material.shininess")
        }
    );
}



template<>
typename MaterialTraits<MaterialDS>::locations_type query_locations<MaterialDS>(ShaderProgram& sp) {
    return MaterialDSLocations{
        .textures = {
            sp.location_of("material.diffuse"),
            sp.location_of("material.specular")
        },
        .shininess = sp.location_of("material.shininess")
    };
}





} // namespace learn

