#pragma once
#include <PerspectiveCamera.hpp>
#include <entt/entity/fwd.hpp>


namespace josh::imguihooks::registry {


/*
FIXME:
It acts as a registry hook even though the camera is not an entity.
Kinda bad, but I need to plug this somewhere.
*/
class PerspectiveCamera {
private:
    josh::PerspectiveCamera& cam_;
public:
    PerspectiveCamera(josh::PerspectiveCamera& cam) : cam_{ cam } {}
    void operator()(entt::registry&);
};


} // namespace josh::imguihooks::registry
