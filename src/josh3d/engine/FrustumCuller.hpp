#pragma once
#include "Mesh.hpp"
#include "tags/Culled.hpp"
#include "components/BoundingSphere.hpp"
#include "components/ChildMesh.hpp"
#include "Transform.hpp"
#include "ViewFrustum.hpp"
#include <entt/entt.hpp>
#include <glm/ext/scalar_common.hpp>
#include <algorithm>
#include <utility>




namespace josh {


class FrustumCuller {
private:
    entt::registry& registry_;

public:
    FrustumCuller(entt::registry& registry)
        : registry_{ registry }
    {}

    template<typename CullTagT = tags::Culled>
    void cull_from_bounding_spheres(const ViewFrustumAsPlanes& frustum) {
        // Assume the frustum has been correctly
        // transformed with the camera's transforms into world-space.

        auto get_full_transform = [&](entt::entity ent, const Transform& transform) {
            if (auto as_child = registry_.try_get<components::ChildMesh>(ent)) {
                return registry_.get<Transform>(as_child->parent) * transform;
            } else {
                return transform;
            }
        };

        auto cullable_meshes_view =
            std::as_const(registry_).view<Mesh, Transform, components::BoundingSphere>();

        for (auto [entity, _, transform, sphere] : cullable_meshes_view.each()) {

            const auto full_transform = get_full_transform(entity, transform);
            const float scaled_radius = sphere.scaled_radius(full_transform.scaling());

            const glm::vec3 sphere_center = full_transform.position();

            auto is_fully_in_front_of = [&](const Plane& plane) {
                // Deliniates the enclosing volume of valid positions for the sphere center.
                const float closest_approach = plane.closest_distance + scaled_radius;

                // Projection of the sphere center onto the normal axis of the plane.
                const float normally_projected_distance = glm::dot(plane.normal, sphere_center);

                // This allows us to work with the distance along the normal axis of the plane.
                return normally_projected_distance > closest_approach;
            };

            const bool should_be_culled =
                is_fully_in_front_of(frustum.near())   ||
                is_fully_in_front_of(frustum.far())    ||
                is_fully_in_front_of(frustum.left())   ||
                is_fully_in_front_of(frustum.right())  ||
                is_fully_in_front_of(frustum.bottom()) ||
                is_fully_in_front_of(frustum.top());

            const bool was_culled = registry_.all_of<CullTagT>(entity);

            // TODO: Add support for additive culling?
            if (should_be_culled != was_culled) {
                if (should_be_culled) /* as it wasn't previously */ {
                    registry_.emplace<CullTagT>(entity);
                } else /* should now be un-culled */ {
                    registry_.erase<CullTagT>(entity);
                }
            }


        }

    }


};






} // namespace josh
