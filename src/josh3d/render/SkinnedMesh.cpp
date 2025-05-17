#include "SkinnedMesh.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <ranges>


namespace josh {


PosedSkeleton::PosedSkeleton(std::shared_ptr<const Skeleton> skeleton)
    : skeleton{ MOVE(skeleton) }
    , M2Js{
        this->skeleton->joints |
        std::views::transform([](const Joint& j) { return inverse(j.inv_bind); }) |
        ranges::to<std::vector>()
    }
    , skinning_mats(this->skeleton->joints.size(), glm::identity<mat4>())
{}


auto Pose::from_skeleton(const Skeleton& skeleton)
    -> Pose
{
    const auto num_joints = skeleton.joints.size();
    Pose pose;
    pose.M2Js.reserve(num_joints);
    pose.skinning_mats.reserve(num_joints);
    for (const Joint& j : skeleton.joints) {
        pose.M2Js.push_back(inverse(j.inv_bind));
        pose.skinning_mats.push_back(glm::identity<mat4>());
    }
    return pose;
}


} // namespace josh
