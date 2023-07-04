#include "MaterialDSN.hpp"
#include "AssimpModelLoader.hpp"
#include "TextureHandlePool.hpp"
#include "GlobalsGL.hpp"
#include <assimp/material.h>
#include <assimp/mesh.h>




namespace learn {



void MaterialDSN::apply(ActiveShaderProgram& asp) const {
    apply(asp, query_locations(asp));
}

void MaterialDSN::apply(ActiveShaderProgram& asp, const locations_type& locations) const {
    using namespace gl;
    diffuse->bind_to_unit_index(0);
    specular->bind_to_unit_index(1);
    normal->bind_to_unit_index(2);
    asp .uniform(locations.diffuse, 0)
        .uniform(locations.specular, 1)
        .uniform(locations.normal, 2)
        .uniform(locations.shininess, shininess);
}

auto MaterialDSN::query_locations(ActiveShaderProgram& asp) -> locations_type {
    return locations_type{
        .diffuse   = asp.location_of("material.diffuse"),
        .specular  = asp.location_of("material.specular"),
        .normal    = asp.location_of("material.normal"),
        .shininess = asp.location_of("material.shininess")
    };
}

auto MaterialDSN::query_locations(ShaderProgram& sp) -> locations_type {
    return locations_type{
        .diffuse   = sp.location_of("material.diffuse"),
        .specular  = sp.location_of("material.specular"),
        .normal    = sp.location_of("material.normal"),
        .shininess = sp.location_of("material.shininess")
    };
}







static Shared<Texture2D> get_texture_from_material(
    const ModelLoadingContext& context,
    const aiMaterial* material, aiTextureType type)
{

    if (material->GetTextureCount(type) == 0) {
        return nullptr;
    }

    aiString filename;
    material->GetTexture(type, 0ull, &filename);

    std::string full_path{ context.directory + filename.C_Str() };

    TextureType tex_type{};
    switch (type) {
        case aiTextureType_DIFFUSE:
            tex_type = TextureType::diffuse; break;
        case aiTextureType_SPECULAR:
            tex_type = TextureType::specular; break;
        case aiTextureType_NORMALS:
            tex_type = TextureType::normal; break;
        case aiTextureType_HEIGHT: // FIXME: This is a hack to make .obj load normals.
            tex_type = TextureType::normal; break;
        default:
            tex_type = TextureType::specular;
    }

    // FIXME: pool should be a c-tor parameter of something.
    // Can be passed through context.
    return globals::texture_handle_pool.load(
        full_path, TextureHandleLoadContext{ tex_type }
    );
}









template<>
MaterialDSN get_material<MaterialDSN>(
    const ModelLoadingContext& context,
    const aiMesh* mesh)
{
    aiMaterial* material =
        context.scene->mMaterials[mesh->mMaterialIndex];

    Shared<Texture2D> diffuse =
        get_texture_from_material(context, material, aiTextureType_DIFFUSE);

    Shared<Texture2D> specular =
        get_texture_from_material(context, material, aiTextureType_SPECULAR);

    Shared<Texture2D> normal =
        get_texture_from_material(context, material, aiTextureType_NORMALS);

    if (!diffuse) { diffuse = globals::default_diffuse_texture; }
    if (!specular) { specular = globals::default_specular_texture; }
    if (!normal) {
        // Try again but from height map (.obj).
        normal = get_texture_from_material(context, material, aiTextureType_HEIGHT);
        if (!normal) {
            normal = globals::default_normal_texture;
        }
    }

    return MaterialDSN{
        .diffuse   = std::move(diffuse),
        .specular  = std::move(specular),
        .normal    = std::move(normal),
        .shininess = 128.f
    };

}





} // namespace learn
