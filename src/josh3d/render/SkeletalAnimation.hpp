#pragma once
#include "Common.hpp"
#include "Skeleton.hpp"
#include "SkeletonStorage.hpp"
#include "Transform.hpp"
#include <glm/ext/quaternion_common.hpp>
#include <memory>


namespace josh {


/*
Per-channel keyframe-based animation clip representation.
Keyframes for Translation, Rotation, and Scaling are stored in separate channels.

TODO: Deprecate.
*/
struct AnimationClip
{
    template<typename T>
    struct Key
    {
        double time;
        T      value;
    };

    struct JointKeyframes
    {
        std::vector<Key<vec3>> t;
        std::vector<Key<quat>> r;
        std::vector<Key<vec3>> s;
    };

    double                          duration;
    std::vector<JointKeyframes>     keyframes;
    std::shared_ptr<const Skeleton> skeleton; // Technically, not used anywhere here, but implicitly depends on it.

    auto num_joints() const noexcept -> size_t { return skeleton->joints.size(); }
    auto sample_at(size_t joint_idx, double time) const noexcept -> Transform;
};


struct Animation2lip
{
    template<typename T>
    struct Key
    {
        double time;
        T      value;
    };

    // TODO: Fairly bad storage.
    struct JointKeyframes
    {
        Vector<Key<vec3>> t;
        Vector<Key<quat>> r;
        Vector<Key<vec3>> s;
    };

    double                 duration;
    Vector<JointKeyframes> keyframes;
    String                 name;
    SkeletonID             skeleton_id;

    auto num_joints() const noexcept -> usize { return keyframes.size(); }
    auto sample_at(uindex joint_idx, double time) const noexcept -> Transform;
};


/*
A hack to connect meshes to their animations.

TODO: Deprecate.
*/
struct MeshAnimations
{
    std::vector<std::shared_ptr<const AnimationClip>> anims;
};


/*
A component that represents an active animation.

TODO: Deprecate.
*/
struct PlayingAnimation
{
    double                               current_time{};
    std::shared_ptr<const AnimationClip> current_anim;
    bool                                 paused      {}; // Hack, should be replaced with another component instead.
};


} // namespace josh
