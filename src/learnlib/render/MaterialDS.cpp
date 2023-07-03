#include "GLObjects.hpp"
#include "GlobalsGL.hpp"
#include "MaterialDS.hpp"
#include "AssimpModelLoader.hpp"
#include "TextureHandlePool.hpp"
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <glbinding/gl/gl.h>


namespace learn {



void MaterialDS::apply(ActiveShaderProgram& asp) const {
    apply(asp, query_locations(asp));
}

void MaterialDS::apply(ActiveShaderProgram& asp, const MaterialDS::locations_type& locations) const {
    using namespace gl;
    diffuse->bind_to_unit(GL_TEXTURE0);
    asp.uniform(locations.diffuse, 0);
    specular->bind_to_unit(GL_TEXTURE1);
    asp.uniform(locations.specular, 1);
    asp.uniform(locations.shininess, this->shininess);
}

MaterialDS::locations_type MaterialDS::query_locations(ActiveShaderProgram& asp) {
    return MaterialDS::locations_type{
        .diffuse = asp.location_of("material.diffuse"),
        .specular = asp.location_of("material.specular"),
        .shininess = asp.location_of("material.shininess")
    };
}

MaterialDS::locations_type MaterialDS::query_locations(ShaderProgram& sp) {
    return MaterialDS::locations_type{
        .diffuse = sp.location_of("material.diffuse"),
        .specular = sp.location_of("material.specular"),
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
        case aiTextureType_DIFFUSE: tex_type = TextureType::diffuse; break;
        case aiTextureType_SPECULAR: tex_type = TextureType::specular; break;
        default: tex_type = TextureType::specular;
    }

    // FIXME: pool should be a c-tor parameter of something.
    // Can be passed through context.
    return globals::texture_handle_pool.load(
        full_path, TextureHandleLoadContext{ tex_type }
    );
}





template<>
MaterialDS get_material<MaterialDS>(
    const ModelLoadingContext& context,
    const aiMesh* mesh)
{

    aiMaterial* material =
        context.scene->mMaterials[mesh->mMaterialIndex];

    Shared<Texture2D> diffuse =
        get_texture_from_material(context, material, aiTextureType_DIFFUSE);

    Shared<Texture2D> specular =
        get_texture_from_material(context, material, aiTextureType_SPECULAR);

    if (!diffuse) { diffuse = globals::default_diffuse_texture; }
    if (!specular) { specular = globals::default_specular_texture; }

    return MaterialDS{
        .diffuse   = std::move(diffuse),
        .specular  = std::move(specular),
        .shininess = 128.f
    };

}







} // namespace learn

