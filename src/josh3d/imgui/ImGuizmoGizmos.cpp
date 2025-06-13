#include "ImGuizmoGizmos.hpp"
#include "AABB.hpp"
#include "Components.hpp"
#include "ContainerUtils.hpp"
#include "SceneGraph.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "Tags.hpp"
#include "Transform.hpp"
#include "Transform.hpp"
#include "UIContext.hpp"
#include "tags/Selected.hpp"
#include <ImGuizmo.h>
#include <entt/entity/fwd.hpp>
#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <cassert>


namespace josh {


void ImGuizmoGizmos::new_frame()
{
    ImGuizmo::BeginFrame();
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
}

/*
When it comes to gizmo display and interaction there's a set of
almost orthogonal questions that must be answered for every selection set.

Apparently, this topic is a little convoluted and there's no one-size-fits-all
solution, that would work "intuitively" from the UX perspective.

Also, nonuniform scaling is a complete torture, as we normally don't have
a way of representing "skew" in the Transform type, so what would be
intuitive from the perspective of linear algebra is not accessble to us.

Here's the options that are available followed by the ones I want to settle on:


1. Where the gizmo is located:
    a. At midpoint of all selected; (Blender)
    b. At some "last selected";

2. How the gizmo is oriented in local mode:
    a. Forbidden to operate on multiple selections in local mode;
    b. As some "last selected"; (Blender)
    c. As an average orientation of all selected (what is that even?);


[1a, 2b] - This is what Blender does, and it seems to work fine there.
This, however, requires that we always keep track of some "last selected",
and all algorithms that do selection have to comply in an intuitive manner.

"Last selected" is also useful for quick parenting, so I like the idea still.


3. How to apply the resulting transformation to the nodes:
    a. Equally to each Transform of each selected;
    b. To the lowest depth selected node(s) of each subtree only; (Blender)


[3b] - Option "a" is not really an option. If you translate both the parent
and the child by the same dx, the child ends up translated by 2*dx, which
is not the most intuitive, or useful, outcome.

Option "b" is reasonable, but will force us to walk the tree up all the time,
and is more cumbersome to implement, but should at least make sense.


4. How to treat rotation of multiple selections:
    a. Around local pivots of each selected; (Blender 1)
    b. Around midpoint of all selected;      (Blender 2)
    c. Around some "last selected" pivot;    (Blender 3)
    d. Around some special pivot object;     (Blender 4, this is the 3D Cursor)


[4b] - Now, here all options make sense in some context, and blender implements
them all successfully, but I'd like to keep my setup relatively small.
Option "b" here seems to me like the most useful one by default, so we'll stick with it.


5. How to treat nonuniform local scaling of multiple selections:
    a. Forbid nonuniform scaling of multiple selections;
    b. Scale locally for each selected;
    c. Reproject scaling coefficient back to local axes of each object; (Blender)

6. How to treat nonuniform world scaling:
    a. Forbid nonuniform world scaling entirely;
    c. Reproject scaling coefficient back to local axes of each object; (Blender)


[5a, 6a] - While Blender has a "solution" for this problem, nonuniform scaling is literal
hell and the Blender solution is not something that is super predictable or intuitive.

We disable nonuniform scaling along world axes, and any nonuniform scaling for multiple
selections. Essentially, every time the scaling axes are not the local basis
of the object, we don't allow that scaling.

EDIT: This is not true for now, as *all* non-uniform scaling is forbidden. For sanity.


7. How to treat *uniform* scaling of multiple selections:
    a. Forbid scaling of multiple selections;
    b. Scale locally for each selected;
    c. Scale around a midpoint, convert to scale + translation; (Blender)


[7c] - Uniform scaling is less problematic, although scaling multiple selections
has the additional challenge that relative distance between points has to be preserved.
This will require translation of all objects w.r.t. the midpoint. Should be doable.


(There may be more options for each point)
*/
void ImGuizmoGizmos::display(
    UIContext&  ui,
    const mat4& view_mat,
    const mat4& proj_mat)
{
    auto& registry = ui.runtime.registry;

    bool debug_window_open = false;
    if (display_debug_window)
        debug_window_open = ImGui::Begin("GizmoDebug");

    // MTransform must have been computed from the scene graph and individual Transforms.
    auto selected = registry.view<Selected, Transform, MTransform>();

    // Size of the dense storage for the leading component.
    // Not every selected will necessarily have a Transform and MTransform, however.
    const auto num_selected = selected.size_hint();

    if (num_selected != 0)
    {
        // Filter the list of selected nodes for the ones that will
        // actually be affected by the transformation.
        //
        // We are searching for "highest common selected ancestors".
        //
        // The algorithm is relatively simple, and somewhat braindead:
        //     1. For each selected node, walk up the tree along the edge until the root;
        //     2. For each node visited in the walk, update the last selected for that edge;
        //     3. After reaching the root, push the highest recorded selected node from each edge into a set;
        //     4. After each selected node is visited, the resulting set will contain all the highest common selected ancestors.
        //
        // NOTE: Keeping a second set of "visited" nodes should remove redundant steps
        // for deep hierarchies, but this is scene-graph, it rarely gets particularly deep.
        // For now, I don't care, maybe when there's a `node_allocator` I'll celebrate by adding
        // a second set.

        thread_local HashSet<Entity> transform_targets;
        transform_targets.clear(); // Ensure that it's clear before pushing new elements.

        for (const Entity entity : selected)
        {
            const CHandle handle = { registry, entity };

            Entity highest_selected{ entity };
            traverse_ancestors_upwards(handle, [&](CHandle ancestor)
            {
                if (has_tag<Selected>(ancestor))
                    highest_selected = ancestor.entity();
            });

            transform_targets.emplace(highest_selected);
        }

        // Bail early if there are no valid transform targets.
        if (transform_targets.empty())
        {
            // TODO: Probably reorder this so that you don't have to call this in 2 places.
            if (display_debug_window)
                ImGui::End();
            return;
        }

        // Locate the gizmo at midpoint of all transform targets.
        vec3 midpoint_world = {}; // Contravariant position of the midpoint in world-space.

        for (const Entity entity : transform_targets)
        {
            const Handle handle = { registry, entity };

            if (preferred_location == GizmoLocation::AABBMidpoint and
                has_component<AABB>(handle))
            {
                // If has world-space AABB, use the midpoint of that.
                midpoint_world += handle.get<AABB>().midpoint();
            } else {
                // Otherwise, use the position of the local origin.
                midpoint_world += selected.get<MTransform>(entity).decompose_position();
            }
        }
        midpoint_world /= transform_targets.size();

        const vec3 gizmo_position = midpoint_world;

        // Get the orientation/scale of the gizmo from the active entity.
        //
        // TODO: We don't have LastSelected or ActiveSelected yet ;_;
        // We'll just grab any of the selected for now.
        const Entity active_entity = *transform_targets.begin();
        const mat3   gizmo_mat3    = selected.get<MTransform>(active_entity).model();

        // Since ImGuizmo tries to be "helpful" by being yada-yada "immediate mode" and
        // modifying the model matrix inplace, we'll have to use a fake model matrix for the gizmo.
        //
        // Because not only do we want to manipulate *multiple* objects,
        // but we also don't want to touch their model matrices, and instead
        // apply the transformation to their local Transforms.
        //
        // So after each manipulation, we'll have to extract the "delta" of that,
        // whatever that means, and correctly reapply that to each entity.

        const mat4 old_gizmo_mat4 = glm::translate(gizmo_position) * mat4(gizmo_mat3);
        mat4       new_gizmo_mat4 = old_gizmo_mat4;

        const ImGuizmo::MODE mode = eval%[&]{
            switch (active_space)
            {
                case GizmoSpace::World: return ImGuizmo::WORLD;
                case GizmoSpace::Local: return ImGuizmo::LOCAL;
                default: panic();
            }
        };

        const ImGuizmo::OPERATION operation = eval%[&]{
            switch (active_operation)
            {
                case GizmoOperation::Translation: return ImGuizmo::TRANSLATE;
                case GizmoOperation::Rotation:    return ImGuizmo::ROTATE;
                case GizmoOperation::Scaling:     return ImGuizmo::SCALE_Y;
                default: panic();
            }
        };

        const bool manipulated =
            ImGuizmo::Manipulate(value_ptr(view_mat), value_ptr(proj_mat),
                operation, mode, value_ptr(new_gizmo_mat4));

        // Some debugging helpers.
        thread_local mat4  last_O2N_gizmo  = {};
        thread_local mat4  last_O2N_world  = {};
        thread_local mat4  last_O2N_median = {};
        thread_local mat4  last_O2N_parent = {};
        thread_local mat4  last_O2N_local  = {};

        thread_local GizmoOperation last_tweak = {};

        thread_local vec3  last_translation_world_dr  = {};
        thread_local vec3  last_translation_parent_dr = {};
        thread_local vec3  last_translation_local_dr  = {};

        thread_local vec3  last_rotation_gizmo_axis   = {};
        thread_local float last_rotation_gizmo_angle  = {};
        thread_local vec3  last_rotation_world_axis   = {};
        thread_local float last_rotation_world_angle  = {};
        thread_local vec3  last_rotation_parent_axis  = {};
        thread_local float last_rotation_parent_angle = {};
        thread_local vec3  last_rotation_local_axis   = {};
        thread_local float last_rotation_local_angle  = {};

        thread_local float last_scaling_factor = {};

        if (debug_window_open)
        {
            ImGui::TextUnformatted("Current Gizmo Matrix:");
            imgui::Matrix4x4DisplayWidget(new_gizmo_mat4);
            ImGui::Separator();

            ImGui::TextUnformatted("Last Tweak Delta (Gizmo):");
            imgui::Matrix4x4DisplayWidget(last_O2N_gizmo);
            ImGui::Text("det(dG) = %.3f", glm::determinant(last_O2N_gizmo));
            ImGui::Separator();

            ImGui::TextUnformatted("Last Tweak Delta (World):");
            imgui::Matrix4x4DisplayWidget(last_O2N_world);
            ImGui::Text("det(dW) = %.3f", glm::determinant(last_O2N_world));
            ImGui::Separator();

            ImGui::TextUnformatted("Last Tweak Delta (Median):");
            imgui::Matrix4x4DisplayWidget(last_O2N_median);
            ImGui::Text("det(dM) = %.3f", glm::determinant(last_O2N_median));
            ImGui::Separator();

            ImGui::TextUnformatted("Last Tweak Delta (Parent):");
            imgui::Matrix4x4DisplayWidget(last_O2N_parent);
            ImGui::Text("det(dP) = %.3f", glm::determinant(last_O2N_parent));
            ImGui::Separator();

            ImGui::TextUnformatted("Last Tweak Delta (Local):");
            imgui::Matrix4x4DisplayWidget(last_O2N_local);
            ImGui::Text("det(dL) = %.3f", glm::determinant(last_O2N_local));
            ImGui::Separator();

            ImGui::BeginDisabled();
            if (last_tweak == GizmoOperation::Translation)
            {
                ImGui::TextUnformatted("Last Tweak: Translation");
                ImGui::InputFloat3("World dr",  value_ptr(last_translation_world_dr) );
                ImGui::InputFloat3("Parent dr", value_ptr(last_translation_parent_dr));
                ImGui::InputFloat3("Local dr",  value_ptr(last_translation_local_dr) );
            }
            else if (last_tweak == GizmoOperation::Rotation)
            {
                ImGui::TextUnformatted("Last Tweak: Rotation");
                vec4 gizmo_aa { last_rotation_gizmo_axis,  last_rotation_gizmo_angle  };
                vec4 world_aa { last_rotation_world_axis,  last_rotation_world_angle  };
                vec4 parent_aa{ last_rotation_parent_axis, last_rotation_parent_angle };
                vec4 local_aa { last_rotation_local_axis,  last_rotation_local_angle  };
                ImGui::InputFloat4("Gizmo Axis/Angle",  value_ptr(gizmo_aa) );
                ImGui::InputFloat4("World Axis/Angle",  value_ptr(world_aa) );
                ImGui::InputFloat4("Parent Axis/Angle", value_ptr(parent_aa));
                ImGui::InputFloat4("Local Axis/Angle",  value_ptr(local_aa) );
            }
            else if (last_tweak == GizmoOperation::Scaling)
            {
                ImGui::TextUnformatted("Last Tweak: Scaling");
                ImGui::InputFloat("Scaling Factor", &last_scaling_factor);
            }
            ImGui::EndDisabled();
        }

        if (manipulated)
        {
            last_tweak = active_operation;

            /*
            In general, we deal with the following spaces:

            World  (W)   - Global "unoriented" space that serves as the hidden root of the scene graph.
            Gizmo  (G|O) - Space the gizmo exists in *before* the manipulation. Origin at midpoint, oriented as the active object.
            Median (M)   - Space with the origin at midpoint, but oriented as world space.
            Parent (P)   - Parent space of each manipulated object. Same as World for roots of the scene graph.
            Local  (L)   - Local space of each manipulated object.

            There also exists one extra space that we don't directly represent any vector in,
            but instead use as a change-of-basis target, as a way to encode the active transformation
            after manipulating the gizmo:

            New Gizmo (N) - Gizmo space *after* the transformation has been applied to the gizmo matrix.

            */


            /*
            We use vec3s to represent vectors, and mat4s to represent transformations.
            Whenever a transformation needs to be applied to a vector, we "clarify"
            whether the vector is covariant or contravariant.

            Covariant vectors undergo change-of-basis A2B as:

                v_B = v_A * A2B

            While contravariant:

                v_B = inverse(A2B) * v_A

            */
         // const auto covariant     = [](const vec3& v) { return vec4(v, 0.f); };
            const auto contravariant = [](const vec3& v) { return vec4(v, 1.f); };


            const mat4 W2G = old_gizmo_mat4;
            const mat4 G2W = inverse(W2G);

            // NOTE: Last column of a homogeneous 4x4 change-of-basis A2B matrix is
            // the position of the origin of basis B as represented in the basis A.
            const mat4 W2M = glm::translate(midpoint_world);
            const mat4 M2W = inverse(W2M);

            // Compute the change-of-basis from old to new gizmo basis after this manipulation.
            // The resulting O2N matrix represents *the* transformation that our manipulation
            // performed in gizmo space.
            const mat4 W2O       = old_gizmo_mat4; // world     -> old_gizmo
            const mat4 O2W       = inverse(W2O);   // old_gizmo -> world
            const mat4 W2N       = new_gizmo_mat4; // world     -> new_gizmo
            const mat4 O2N_gizmo = O2W * W2N;      // old_gizmo -> new_gizmo

            // O2Ns are treated as an active transformation (linear map) here.
            // Matrix conjugation (aka. similarity) is used to convert this transformation
            // between different bases/spaces (gizmo, world, median, parent, local).
            //
            // See: https://en.wikipedia.org/wiki/Matrix_similarity
            //
            const mat4 O2N_world  = W2G * O2N_gizmo * G2W;
            const mat4 O2N_median = M2W * O2N_world * W2M;

            last_O2N_gizmo  = O2N_gizmo;
            last_O2N_world  = O2N_world;
            last_O2N_median = O2N_median;


            for (const Entity entity : transform_targets)
            {
                const Handle handle = { registry, entity };

                const MTransform& mtransform = handle.get<MTransform>();

                /*
                Because the covariant transformations are applied left-to-right,
                we "chain" change-of-basis transformations left-to-right too.

                Effectively, just swap letters on inverse and cancel adjacent letters:

                    W2L * P2L^-1 =
                    W2L * L2P    = W2P

                    W2G^-1 * W2L * P2L^-1 =
                    G2W    * W2L * L2P    = G2P

                */

                const mat4 W2L = mtransform.model();
                const mat4 L2W = inverse(W2L);

                const mat4 P2L = handle.get<Transform>().mtransform().model();
                const mat4 L2P = inverse(P2L);

                const mat4 W2P = W2L * L2P;
             // const mat4 P2W = inverse(W2P);

                const mat4 M2P = M2W * W2P;
                const mat4 P2M = inverse(M2P);


                const mat4 O2N_local  = L2W * O2N_world * W2L;
                const mat4 O2N_parent = P2L * O2N_local * L2P;

                last_O2N_local  = O2N_local;
                last_O2N_parent = O2N_parent;

                if (active_operation == GizmoOperation::Translation)
                {
                    /*
                    Translation delta is taken from the parent space, because
                    the position field of the Transform is the origin of local
                    space as seen from parent space.

                    Root nodes, for example, have World as their parent space.
                    The positions of root nodes == offsets from world origin.
                    */

                    // The O2N transformations already encode the translation in each space.
                    const vec3 dr_world  = vec3(O2N_world[3] );
                    const vec3 dr_parent = vec3(O2N_parent[3]);
                    const vec3 dr_local  = vec3(O2N_local[3] );

                    handle.get<Transform>().translate(dr_parent);

                    last_translation_world_dr  = dr_world;
                    last_translation_parent_dr = dr_parent;
                    last_translation_local_dr  = dr_local;
                }
                else if (active_operation == GizmoOperation::Rotation)
                {
                    /*
                    We want to rotate around the midpoint of all the objects,
                    which means that the operation is actually a combination
                    of a 2 transformations:

                        1. Position of each pivot has to be rotated around the midpoint.
                        2. Orientation of each object has to be rotated around the local axis.

                    When only one object is selected, the midpoint is equal to pivot,
                    so no translation takes place.
                    */

                    // Translation:

                    // `r` is the local "pivot" point of each object - the origin of the local frame.
                    // It is taken as a position in world-space, since that's what is encoded in the "model/world" matrix.
                    const vec3 r_world = mtransform.decompose_position(); // Contravariant

                    // Median space is a tangent space in world. The covariant "midpoint to r" vector
                    // in world is numerically the same as the contravariant `r` in median space,
                    // since the orientation and scaling of these spaces is the same.
                    //
                    // As such the following transformation is equivalent to this:
                    //
                    //     const vec3 midpoint_to_r_world = r_world - midpoint_world; // Covariant
                    //     const vec3 r_median            = midpoint_to_r_world;      // Contravariant
                    //
                    // NOTE: Inverse of "world->median" change-of-basis is used for contravariant vectors.
                    const vec3 r_median = M2W * contravariant(r_world);

                    // To rotate the pivots around the midpoint, we need to represent
                    // each pivot in the median space, and apply a rotation to each
                    // contravariant pivot vector, "equivalent" to a rotation applied
                    // in the local space of each object.
                    //
                    // NOTE: Active transformation applied on a contravariant vector
                    // is a double-inverse, and is equal to the change-of-basis.
                    const vec3 r_new_median = O2N_median * contravariant(r_median); // Contravariant

                    // The new rotated pivot represents the new `r` in median space,
                    // which needs to be now propagated to the local Transform of each
                    // manipulated objet.
                    //
                    // This can be done in 2 ways:
                    //
                    //     1. Changing the basis of the new contravariant `r` from median space
                    //     to parent space of each object, and resetting the position with that value.
                    //
                    //     2. Computing a covariant delta `r_new - r_old` in the median space,
                    //     then transforming that delta into the parent space, and applying the
                    //     delta to the local Transform.
                    //
                    // NOTE: Option 2 does not work for groups when uniform scaling is applied to the
                    // parent. Not sure why, I must be missing something there.
                    const vec3 r_new_parent = P2M * contravariant(r_new_median); // Contravariant

                    handle.get<Transform>().position() = r_new_parent;

                    // Rotation:

                    const quat  rotation_gizmo_quat = quat_cast(O2N_gizmo);
                    const vec3  axis_gizmo          = axis (rotation_gizmo_quat);
                    const float angle_gizmo         = angle(rotation_gizmo_quat);

                    const quat  rotation_world_quat = quat_cast(O2N_world);
                    const vec3  axis_world          = axis(rotation_world_quat);
                    const float angle_world         = angle(rotation_world_quat);

                    const quat  rotation_parent_quat = quat_cast(O2N_parent);
                    const vec3  axis_parent          = axis(rotation_parent_quat);
                    const float angle_parent         = angle(rotation_parent_quat);

                    const quat  rotation_local_quat = quat_cast(O2N_local);
                    const vec3  axis_local          = axis(rotation_local_quat);
                    const float angle_local         = angle(rotation_local_quat);

                    handle.get<Transform>().rotate(angle_world, axis_local);
                    //
                    // NOTE: This ^ wierd mix of local axis and world angle seems to work even
                    // when nonuniform scales are involved. Sometimes.
                    //
                    // The only problem is that for when the rotation happens in the LOCAL
                    // basis, the basis vectors "adjust" to the skew introduced due to nonuniform
                    // scaling in the parent space, and this seems to screw with the gizmo itself,
                    // that is, it does not interpret the the motion of the mouse correctly in a skewed basis.
                    //
                    // When the gizmo is manipulated in WORLD, then the gizmo itself is static,
                    // and the rotation is actually quite stable.
                    //
                    // Either way, nonuniform scaling sucks, and is probably not worth it.
                    // On the other hand, forcing the gizmo basis to be skew-less, by
                    // orthonormalizing it, for example, could also be tested as an option.
                    // Not sure if that's doable though.

                    // NOTE 2: Okay, I just tried this "local rotaion in skewed basis" in Blender
                    // and I had no idea you could call that "rotation around an axis".
                    // That was the most nonlinear "linear" transformation I've seen.
                    // Incredible. Truly marvellous.
                    //
                    // Yeah, screw nonuniform scaling.

                    // NOTE: This works for everything except *all* nonuniform scaling:
                    // handle.get<Transform>().rotate(rotation_local_quat);

                    last_rotation_gizmo_axis   = axis_gizmo;
                    last_rotation_gizmo_angle  = angle_gizmo;
                    last_rotation_world_axis   = axis_world;
                    last_rotation_world_angle  = angle_world;
                    last_rotation_parent_axis  = axis_parent;
                    last_rotation_parent_angle = angle_parent;
                    last_rotation_local_axis   = axis_local;
                    last_rotation_local_angle  = angle_local;
                }
                else if (active_operation == GizmoOperation::Scaling)
                {
                    /*
                    Uniform scaling only.

                    Similar to rotation, scaling around a midpoint is a combination of
                    a translation and local scaling.

                    Except that due to how we use the gizmo, the matrix does not represent
                    uniform scaling at all. Instead it only scales Y axis in gizmo space.

                    So we need to get the matrix in gizmo space, make it uniform,
                    and then transform that to other spaces.
                    */

                    const float scale_factor_gizmo = O2N_gizmo[1][1];
                    const mat4 O2N_uniform_gizmo  = glm::scale(vec3(scale_factor_gizmo));
                    const mat4 O2N_uniform_world  = W2G * O2N_uniform_gizmo * G2W;
                    const mat4 O2N_uniform_median = M2W * O2N_uniform_world * W2M;
                    const mat4 O2N_uniform_local  = L2W * O2N_uniform_world * W2L;
                    const mat4 O2N_uniform_parent = P2L * O2N_uniform_local * L2P;

                    // TODO: Is the above completely redundant? Uniform scaling is uniform independant
                    // of the basis chosen. Or is this useful when some bases are already non-uniform?

                    // Keep last_O2N_gizmo to represent the initial, non-uniform matrix.
                    last_O2N_world  = O2N_uniform_world;
                    last_O2N_median = O2N_uniform_median;
                    last_O2N_parent = O2N_uniform_parent;
                    last_O2N_local  = O2N_uniform_local;

                    // Translation:
                    //
                    // The steps for translation are very similar to how it's done in rotation.
                    const vec3 r_world      = mtransform.decompose_position();
                    const vec3 r_median     = M2W * contravariant(r_world);
                    const vec3 r_new_median = O2N_uniform_median * contravariant(r_median);
                    const vec3 r_new_parent = P2M * contravariant(r_new_median);

                    handle.get<Transform>().position() = r_new_parent;

                    const float uniform_scale_factor = O2N_uniform_local[1][1];

                    // Overall, this is janky when the local basis has skew.
                    // That is, the parent basis has non-uniform scaling applied.
                    // Whatever. Who cares?

                    handle.get<Transform>().scale(vec3(uniform_scale_factor));

                    last_scaling_factor = uniform_scale_factor;
                } // if (active_operation == ...)
            } // for (entity)
        } // if (manipulated)

        // Clear heap-allocated memory now, so that it's less likely to
        // interleave with other allocations following this.
        // TODO: This is a good reason to write the `node_allocator`
        // and have my peace already.
        transform_targets.clear();
    }

    if (display_debug_window)
        ImGui::End();
}


} // namespace josh






