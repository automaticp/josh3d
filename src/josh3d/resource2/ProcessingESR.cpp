#include "ExternalScene.hpp"
#include "Processing.hpp"


namespace josh {

void unitarize_external_scene(
    esr::ExternalScene& scene,
    Unitarization       algorithm)
{
    for (auto [node_id, node] : scene.view<esr::Node>().each())
    {
        if (node.entities.size() > 1)
        {
            if (algorithm == Unitarization::InsertDummy)
            {
                // Given that the number of entities in the node is N,
                // create N child leaf nodes and move each entity into
                // them one-to-one. The transform is preserved for this
                // node, and the transforms of the new children are Identity.
                while (node.entities.size() > 0)
                {
                    auto [new_child_id, new_child] = scene.create_as<esr::Node>({
                        .name       = node.name,
                        .entities   = {},
                        .transform  = {},
                        .parent_id  = node_id,
                        .child0_id  = esr::null_id,
                        .sibling_id = node.child0_id,
                    });
                    node.child0_id = new_child_id;
                    new_child.entities.push_back(pop_back(node.entities));
                }
            }

            if (algorithm == Unitarization::UnwrapToEdge)
            {
                // Given N entitites in the node, create child a node,
                // then a child of child, then a child of that, etc.
                // until there's a node per entity (N-1 descendents total).
                //
                // NOTE: The resulting order does not matter since the order
                // in the original entities list is just as arbitrary.
                esr::NodeID parent_id = node_id;
                while (node.entities.size() > 1)
                {
                    auto& parent = scene.get<esr::Node>(parent_id);
                    auto [new_child_id, new_child] = scene.create_as<esr::Node>(esr::Node{
                        .name         = node.name,
                        .entities     = {},
                        .transform    = {},
                        .parent_id    = parent_id,
                        .child0_id    = parent.child0_id, // Karen takes the children.
                        .sibling_id   = esr::null_id,
                    });
                    parent.child0_id = new_child_id;
                    parent_id        = new_child_id;

                    // NOTE: Pop from the original node, it has the full list, not the parent.
                    new_child.entities.push_back(pop_back(node.entities));
                }
            }
        }
    } // for (node)
}

} // namespace josh
