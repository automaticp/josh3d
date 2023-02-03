#pragma once
#include "Tile.hpp"
#include "GlobalsUtil.hpp"
#include "Transform2D.hpp"
#include <box2d/b2_body.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_joint.h>
#include <box2d/b2_math.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_settings.h>
#include <box2d/b2_world.h>
#include <box2d/b2_world_callbacks.h>
#include <box2d/box2d.h>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <vector>



// box2d recommends to have it's objects in the scale from 0.1 to 10 (meters).
// For a screen space of 1600 by 900 we just scale it down by 100.

inline b2Vec2 to_world(const glm::vec2& screen_crds) noexcept {
    return b2Vec2{ 0.01f * screen_crds.x, 0.01f * screen_crds.y };
}

inline glm::vec2 to_screen(const b2Vec2& world_crds) noexcept {
    return glm::vec2{ world_crds.x, world_crds.y } * 100.f;
}

inline float to_world(float screen_crds) noexcept {
    return screen_crds * 0.01f;
}

inline float to_screen(float world_crds) noexcept {
    return world_crds * 100.f;
}




struct PhysicsComponent {
    b2Body* body;

    glm::vec2 get_velocity() const noexcept {
        return to_screen(body->GetLinearVelocity());
    }

    void set_velocity(const glm::vec2& v) noexcept {
        body->SetLinearVelocity(to_world(v));
    }

    glm::vec2 get_position() const noexcept {
        return to_screen(body->GetPosition());
    }

    void set_position(const glm::vec2& pos) noexcept {
        body->SetTransform(to_world(pos), body->GetAngle());
    }

};





class ContactListener final : public b2ContactListener {
private:
    std::vector<entt::entity> tiles_to_destroy_;
    entt::registry& registry_;

public:
    ContactListener(entt::registry& registry)
        : registry_{ registry }
    {
        // No way this many actually collide in a single step.
        tiles_to_destroy_.reserve(8);
    }
    // void BeginContact(b2Contact* contact) final {}

    void EndContact(b2Contact* contact) final {
        auto ent_a = get_entity(contact->GetFixtureA()->GetBody());
        auto ent_b = get_entity(contact->GetFixtureB()->GetBody());

        TileComponent* tile_a = registry_.try_get<TileComponent>(ent_a);
        if (tile_a && tile_a->type != TileType::solid) {
            tiles_to_destroy_.push_back(ent_a);
        }

        TileComponent* tile_b = registry_.try_get<TileComponent>(ent_b);
        if (tile_b && tile_b->type != TileType::solid) {
            tiles_to_destroy_.push_back(ent_b);
        }
    }

    void remove_destroyed_tiles() noexcept {
        if (!tiles_to_destroy_.empty()) {
            registry_.destroy(tiles_to_destroy_.begin(), tiles_to_destroy_.end());
            tiles_to_destroy_.clear();
        }
    }
    // void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) final { /* handle pre-solve event */ }
    // void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) final { /* handle post-solve event */ }

private:
    static entt::entity get_entity(b2Body* body) noexcept {
        return static_cast<entt::entity>(body->GetUserData().pointer);
    }
};



class PhysicsSystem {
private:
    entt::registry& registry_;
    b2World world_;
    ContactListener contact_listener_;

public:
    PhysicsSystem(entt::registry& registry)
        : registry_{ registry }
        , world_{ b2Vec2{ 0.f, 0.f } }
        , contact_listener_{ registry }
    {
        world_.SetContactListener(&contact_listener_);

        // Well, that's one way to express destroying physics bodies when the entity is destroyed.
        // But I wonder if defining move-ctor/destructor for PhysicsComponent would be better?
        // Then it's probably not trivially-something anymore and that's eugh...
        registry.on_destroy<PhysicsComponent>().connect<&PhysicsSystem::destroy_body>(*this);
    }


    [[nodiscard]] b2Joint* weld(entt::entity ent1, entt::entity ent2) {
        b2WeldJointDef def;
        auto& p1 = registry_.get<PhysicsComponent>(ent1);
        auto& p2 = registry_.get<PhysicsComponent>(ent2);
        def.bodyA = p1.body;
        def.bodyB = p2.body;
        def.localAnchorA = p2.body->GetPosition() - p1.body->GetPosition();
        return world_.CreateJoint(&def);
    }

    void unweld(b2Joint* joint) {
        world_.DestroyJoint(joint);
    }


    [[nodiscard]] PhysicsComponent create_wall(
        entt::entity entity, const glm::vec2& pos, const glm::vec2& scale);

    [[nodiscard]] PhysicsComponent create_tile(
        entt::entity entity, const glm::vec2& pos, const glm::vec2& scale);

    [[nodiscard]] PhysicsComponent create_paddle(
        entt::entity entity, const glm::vec2& pos, const glm::vec2& scale);

    [[nodiscard]] PhysicsComponent create_ball(
        entt::entity entity, const glm::vec2& pos, float radius_screen);


    void update(entt::registry& registry, float time_step) {

        world_.Step(time_step, 8, 3);

        // FIXME: This is probably better to be implemented as a game event...
        // Especially later, when powerups come into play.
        contact_listener_.remove_destroyed_tiles();

    }


private:
    [[nodiscard]] b2Body* make_body(entt::entity ent, b2BodyType type, const glm::vec2& pos);
    void destroy_body(entt::registry& reg, entt::entity ent);
};
