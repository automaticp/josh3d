#include "PhysicsSystem.hpp"
#include <box2d/b2_body.h>
#include <box2d/box2d.h>


b2Body* PhysicsSystem::make_body(
    entt::entity ent, b2BodyType type, const glm::vec2& pos) {
    b2BodyDef def;
    def.type = type;
    def.position = to_world(pos);
    def.fixedRotation = true;
    def.userData.pointer = static_cast<uintptr_t>(ent);
    return world_.CreateBody(&def);
}


void PhysicsSystem::destroy_body(entt::registry& reg, entt::entity ent) {
    b2Body* body = reg.get<PhysicsComponent>(ent).body;
    world_.DestroyBody(body);
}



using namespace collision;


PhysicsComponent PhysicsSystem::create_wall(
    entt::entity entity, const glm::vec2& pos, const glm::vec2& scale) {
    b2Body* body = make_body(entity, b2_staticBody, pos);

    b2PolygonShape box_shape;
    b2Vec2 half_scale = to_world(scale * 0.5f);
    box_shape.SetAsBox(half_scale.x, half_scale.y);

    b2FixtureDef fixture_def;
    fixture_def.friction = 0.0f;
    fixture_def.shape = &box_shape;

    fixture_def.filter.categoryBits = category::wall;
    fixture_def.filter.maskBits = mask::wall;

    body->CreateFixture(&fixture_def);

    return { body };
}


PhysicsComponent PhysicsSystem::create_tile(
    entt::entity entity, const glm::vec2& pos, const glm::vec2& scale) {
    b2Body* body = make_body(entity, b2_staticBody, pos);

    b2PolygonShape box_shape;
    b2Vec2 half_scale = to_world(scale * 0.5f);
    box_shape.SetAsBox(half_scale.x, half_scale.y);

    b2FixtureDef fixture_def;
    fixture_def.friction = 0.0f;
    fixture_def.shape = &box_shape;

    fixture_def.filter.categoryBits = category::tile;
    fixture_def.filter.maskBits = mask::tile;

    body->CreateFixture(&fixture_def);

    return { body };
}


PhysicsComponent PhysicsSystem::create_paddle(
    entt::entity entity, const glm::vec2& pos, const glm::vec2& scale) {
    b2Body* body = make_body(entity, b2_kinematicBody, pos);

    b2PolygonShape box_shape;
    b2Vec2 half_scale = to_world(scale * 0.5f);
    box_shape.SetAsBox(half_scale.x, half_scale.y);

    b2FixtureDef fixture_def;
    fixture_def.friction = 0.3f;
    fixture_def.shape = &box_shape;

    fixture_def.filter.categoryBits = category::paddle;
    fixture_def.filter.maskBits = mask::paddle;

    body->CreateFixture(&fixture_def);

    return { body };
}


PhysicsComponent PhysicsSystem::create_ball(
    entt::entity entity, const glm::vec2& pos, float radius_screen) {
    b2Body* body = make_body(entity, b2_dynamicBody, pos);

    b2CircleShape circle_shape;
    circle_shape.m_radius = to_world(radius_screen);

    b2FixtureDef fixture_def;
    fixture_def.density = 1.0f;
    fixture_def.friction = 0.2f;
    fixture_def.restitution = 1.0f;
    fixture_def.shape = &circle_shape;

    fixture_def.filter.categoryBits = category::ball;
    fixture_def.filter.maskBits = mask::ball;

    body->CreateFixture(&fixture_def);

    return { body };
}


PhysicsComponent PhysicsSystem::create_powerup(
    entt::entity entity, const glm::vec2& pos, const glm::vec2& scale) {

    // Powerups are made dynamic to participate
    // in the collisions with the floor/paddle.
    b2Body* body = make_body(entity, b2_dynamicBody, pos);

    b2PolygonShape box_shape;
    b2Vec2 half_scale = to_world(scale * 0.5f);
    box_shape.SetAsBox(half_scale.x, half_scale.y);

    b2FixtureDef fixture_def;
    fixture_def.friction = 0.0f;
    fixture_def.density = 1.0f;
    fixture_def.restitution = 1.0f;
    fixture_def.shape = &box_shape;

    fixture_def.filter.categoryBits = category::powerup;
    fixture_def.filter.maskBits = mask::powerup;

    body->CreateFixture(&fixture_def);

    return { body };
}
