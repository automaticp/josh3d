#include "AnimationSystem.hpp"
#include "RenderEngine.hpp"
#include "SkeletalAnimation.hpp"
#include "SkinnedMesh.hpp"
#include "Transform.hpp"


namespace josh::stages::precompute {


void AnimationSystem::operator()(
    RenderEnginePrecomputeInterface& engine)
{
    auto& registry = engine.registry();

    for (auto [e, skinned_mesh, playing]
        : registry.view<SkinnedMesh, PlayingAnimation>().each())
    {
        if (playing.paused) continue; // Ugly hack.
        assert(playing.current_anim->skeleton.get() == skinned_mesh.skeleton.get());
        const auto& anim     = *playing.current_anim;
        const auto  time     = playing.current_time;
        const auto  duration = anim.clock.duration();
        const auto  dt       = engine.frame_timer().delta();

        /*
        Mesh space  - local space of the mesh (aka. model space);
        Bind space  - local space of the joint in bind pose;
        Joint space - local space of the joint at an arbitrary moment;

        Our goal is to transform vertex originally attached to the Bind pose,
        to its position during an active animation that transforms each Joint.

        This is an active transfromation within Mesh space, and is commonly
        referred to as "the skinning matrix".

        What we have is:

        M2B - (Mesh->Bind) CoB. Bind pose transform.
              Transforms contravariant vecs from Bind to Mesh.
              This is analogous to the W2L model matrix of a mesh
              which transforms contravariant vecs from Local to World.

        B2M - (Bind->Mesh) CoB. Inverse bind pose transform.
              This is given in the skeleton data.

        M2J - (Mesh->Joint) CoB. Representation of an animated joint
              in Mesh space. This is computed by walking pose transfroms
              of each joint and chaining L2P (Local->Parent) transformations
              towards root.

        Composing a new CoB:

            B2J = B2M * M2J

        This would represent covariant vecs from Bind space in Joint space.

        However, if we treat this as an *active* transformation,
        this would transform *contravariant* vecs as if they are "attached"
        to the changing basis *in Bind space*.

        An active transformation (here annotated with the space it belongs to as B2J[@B], B2J[@M], etc.)
        can be transformed from Bind space to Mesh space according to the CoB of a linear map:

            B2J[@M] = M2B * B2J[@B] * B2M

        The B2J[@M] is the skinning matrix we're looking for.

        If we expand the B2J[@B] back to its product form, we can simplify:

            B2J[@M] = M2B * (B2M * M2J) * B2M
                    = M2J * B2M

        This means that we only need the chanined transforms of each joint for the current pose,
        and inverse bind matrix of the skeleton to compute the final skinning matrix.


        To clarify, the M2J is computed by a chain-product of local joint transforms
        in the joint tree. Each local joint transform represents a P2L (Parent->Local)
        matrix (Local here is equivalent to Joint, if considering the current joint).

        To get from Mesh to Joint space, we need to chain multiply P2Ls of each joint
        between Mesh space and the relevant joint N:

            M2J = P2L_(0) * P2L_(1) * ... P2L_(N-1) * P2L_(N)
                = M2L_(0) * P2L_(1) * ... P2L_(N-1) * P2J_(N) // M and J are substituted where relevant.

        */

        // Joints are stored in pre-order, meaning that for each node visited
        // all of it's ancestors have already been visited before.
        // This is useful, as it allows us to compute the M2J "top-down" from the root.
        const std::span joints = playing.current_anim->skeleton->joints;

        thread_local std::vector<mat4> M2Js; M2Js.resize(joints.size());

        // Joint with index 0 is always root, we compute it separately, since it has no parent.
        const Transform root_tf = anim.sample_at(0, time);
        M2Js[0] = root_tf.mtransform().model();

        for (size_t j{ 1 }; j < joints.size(); ++j) {
            const Transform joint_tf = anim.sample_at(j, time);
            const mat4 P2J = joint_tf.mtransform().model();
            const mat4 M2P = M2Js[joints[j].parent_id];
            M2Js[j] = M2P * P2J;
        }

        // Fill out the skinning matrices. That's our job.
        skinned_mesh.skinning_mats.clear();
        for (size_t j{ 0 }; j < joints.size(); ++j) {
            const mat4 B2M = joints[j].inv_bind;
            const mat4 M2J = M2Js[j];

            const mat4 skinning_mtf = M2J * B2M; // See notes above for why this looks like this.
            skinned_mesh.skinning_mats.emplace_back(skinning_mtf);
        }

        // Advance the clock forward, and possibly, destroy the PlayingAnimation if it's over.
        playing.current_time = time + dt;
        if (playing.current_time >= duration) {
            registry.erase<PlayingAnimation>(e);
        }
    }

}



} // namespace josh::stages::precompute
