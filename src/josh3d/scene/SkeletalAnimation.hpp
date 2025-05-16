#pragma once
#include "ContainerUtils.hpp"
#include "Skeleton.hpp"
#include "Transform.hpp"
#include <glm/ext/quaternion_common.hpp>
#include <memory>
#include <ranges>


namespace josh {


/*
Per-channel keyframe-based animation clip representation.
Keyframes for Translation, Rotation, and Scaling are stored in separate channels.
*/
struct AnimationClip {
    template<typename T>
    struct Key {
        double time;
        T      value;
    };

    struct JointKeyframes {
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


/*
A hack to connect meshes to their animations.
*/
struct MeshAnimations {
    std::vector<std::shared_ptr<const AnimationClip>> anims;
};


/*
A component that represents an active animation.
*/
struct PlayingAnimation {
    double                               current_time{};
    std::shared_ptr<const AnimationClip> current_anim;
    bool                                 paused      {}; // Hack, should be replaced with another component instead.
};




inline auto AnimationClip::sample_at(size_t joint_idx, double time) const noexcept
    -> Transform
{
    // 1. Find 2 nearest keyframes for each channel.
    // 2. Interpolate according to channel type.

    // TODO: Should custom interpolation modes be supported per-keyframe?
    // TODO: Could the binary search be accelerated with a "hint" parameter?

    auto interpolate = [&](auto&& range, double time, auto&& interp_func) {
        using std::views::transform;
        auto get_time = [](const auto& sample) { return sample.time; };
        const auto [prev_idx, next_idx, s] = binary_search(range | transform(get_time), time);
        const auto prev = range[prev_idx].value;
        const auto next = range[next_idx].value;
        return interp_func(prev, next, s);
    };

    auto lerp_vec3  = [](const vec3& v1, const vec3& v2, float s) { return glm::lerp(v1, v2, s);  };
    auto slerp_quat = [](const quat& q1, const quat& q2, float s) { return glm::slerp(q1, q2, s); };

    const auto& channels = keyframes[joint_idx];

    const vec3 position = channels.t.size() ? interpolate(channels.t, time, lerp_vec3)  : vec3(0.f, 0.f, 0.f);
    const quat rotation = channels.r.size() ? interpolate(channels.r, time, slerp_quat) : quat(0.f, 0.f, 0.f, 1.f);
    const vec3 scale    = channels.s.size() ? interpolate(channels.s, time, lerp_vec3)  : vec3(1.f, 1.f, 1.f);

    return { position, rotation, scale };
}


} // namespace josh
