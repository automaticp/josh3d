#pragma once
#include "Skeleton.hpp"
#include "Transform.hpp"


namespace josh {


class AnimationClock {
public:
    // TODO: This time remappings are a mess, figure it out better.
    AnimationClock(double duration, double delta)
        : duration_   { duration }
        , delta_      { delta    }
        , num_samples_{ size_t(duration / delta) + 1 }
    {
        assert(duration_    > 0);
        assert(delta_       > 0);
    }

    auto duration()    const noexcept -> double { return duration_;        }
    auto delta()       const noexcept -> double { return delta_;           }
    auto fps()         const noexcept -> double { return 1.0 / delta_;     }
    auto num_samples() const noexcept -> size_t { return num_samples_;     }
    auto num_frames()  const noexcept -> size_t { return num_samples_ - 1; }
    auto time_of_sample(size_t i) const noexcept -> double { return delta_ * double(i); }

private:
    double duration_;
    double delta_;
    size_t num_samples_;
};


/*
Fixed tickrate animation clip representation. 30-ticks per second.
*/
struct SkeletalAnimation {
    struct Sample {
        std::unique_ptr<Transform[]> joint_poses;
    };

    AnimationClock                  clock;
    std::vector<Sample>             samples;
    std::shared_ptr<const Skeleton> skeleton;

    auto num_joints() const noexcept -> size_t { return skeleton->joints.size(); }
    auto sample_at(size_t joint_idx, double time) const noexcept -> Transform;
    // TODO: Batch populate a pose array?
};


/*
A hack to connect meshes to their animations.
*/
struct MeshAnimations {
    std::vector<std::shared_ptr<const SkeletalAnimation>> anims;
};


/*
A component that represents an active animation.
*/
struct PlayingAnimation {
    double                                   current_time{};
    std::shared_ptr<const SkeletalAnimation> current_anim;
    bool                                     paused      {}; // Hack, should be replaced with another component instead.
};


inline auto SkeletalAnimation::sample_at(size_t joint_idx, double time) const noexcept
    -> Transform
{
    // 1. Find 2 nearest sample indexes by time.
    // 2. lerp, slerp and logerp.
    assert(time < clock.duration()); // This is a bit weird.
    const size_t prev_idx = size_t((time / clock.duration()) * double(clock.num_frames()));
    const size_t next_idx = prev_idx + 1;

    const double s = // Interpolation coefficient.
        (time - clock.time_of_sample(prev_idx)) /
            (clock.time_of_sample(next_idx) - clock.time_of_sample(prev_idx));

    const Transform prev_tf = samples[prev_idx].joint_poses[joint_idx];
    const Transform next_tf = samples[next_idx].joint_poses[joint_idx];

    return {
        glm::lerp (prev_tf.position(),    next_tf.position(),    float(s)),
        glm::slerp(prev_tf.orientation(), next_tf.orientation(), float(s)),
        glm::exp(glm::lerp(glm::log(prev_tf.scaling()), glm::log(next_tf.scaling()), float(s))),
    };
}


} // namespace josh
