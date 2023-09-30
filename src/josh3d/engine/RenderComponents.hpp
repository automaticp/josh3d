#pragma once
#include "Filesystem.hpp"
#include "VPath.hpp"
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "Model.hpp"
#include "Shared.hpp"
#include <entt/entity/entity.hpp>
#include <string>




namespace josh {


/*
Empty types modelling boolean conditions and inclusion/exclusion.
*/
namespace tags {


/*
Per-mesh tag component that enables alpha-testing in shadow and geometry mapping.
*/
struct AlphaTested {};


/*
Empty component used to enable shadows being cast from various light sources.
*/
struct ShadowCasting {};


/*
Tag type denoting objects that were culled from rendering.
*/
struct Culled {};


/*
Tag type denoting objects that were culled from directional shadow mapping.
*/
struct CulledFromCascadedShadowMapping {};


} // namespace tags




/*
WIP

I'll slowly fill out this with wrappers and aliases
to have a semi-standard list of components used in rendering.
*/
namespace components {


/*
Simple pivot-centered sphere that fully encloses and object.
*/
struct BoundingSphere {
    float radius;

    float scaled_radius(const glm::vec3& scale) const noexcept {
        return glm::max(glm::max(scale.x, scale.y), scale.z) * radius;
    }
};



struct MaterialDiffuse {
    Shared<UniqueTexture2D> diffuse;
};

struct MaterialSpecular {
    Shared<UniqueTexture2D> specular;
    GLfloat shininess{ 128.f };
};

struct MaterialNormal {
    Shared<UniqueTexture2D> normal;
};




struct Skybox {
    Shared<UniqueCubemap> cubemap;
};




struct Name {
    std::string name;
};


using josh::Path;

using josh::VPath;


struct ChildMesh {
    entt::entity parent;

    ChildMesh(entt::entity parent_entity)
        : parent{ parent_entity }
    {
        assert(parent != entt::null);
    }
};


using Model = ModelComponent;




} // namespace components


} // namespace josh
