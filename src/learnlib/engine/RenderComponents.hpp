#pragma once
#include "GLObjects.hpp"
#include "Model.hpp"
#include "Shared.hpp"
#include <entt/entity/entity.hpp>
#include <string>




namespace learn::components {

/*
WIP

I'll slowly fill out this with wrappers, tags and aliases
to have a semi-standard list of components used in rendering
*/


/*
Empty component used to enable shadows being cast from various light sources.
*/
struct ShadowCasting {};


struct Skybox {
    Shared<Cubemap> cubemap;
};



struct Name {
    std::string name;
};


struct Path {
    std::string path;
};


struct ChildMesh {
    entt::entity parent;

    ChildMesh(entt::entity parent_entity)
        : parent{ parent_entity }
    {
        assert(parent != entt::null);
    }
};


using Model = ModelComponent;


} // namespace learn::components
