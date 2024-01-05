#pragma once
#include <entt/entity/entity.hpp>


namespace josh::components {


struct ChildMesh {
    entt::entity parent;

    ChildMesh(entt::entity parent_entity)
        : parent{ parent_entity }
    {
        assert(parent != entt::null);
    }
};


} // namespace josh::components
