#pragma once
#include "SkeletalAnimation.hpp"


namespace josh {


/*
A hack to connect meshes to their animations.

TODO: Deprecate.
*/
struct MeshAnimations
{
    Vector<std::shared_ptr<const AnimationClip>> anims;
};

/*
A component that represents an active animation.

TODO: Deprecate.
*/
struct PlayingAnimation
{
    double                               current_time = {};
    std::shared_ptr<const AnimationClip> current_anim;
    bool                                 paused       = {}; // Hack, should be replaced with another component instead.
};


} // namespace josh
