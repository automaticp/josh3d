#pragma once
#include "PostprocessDoubleBuffer.hpp"
#include "PostprocessRenderer.hpp"
#include "GLObjects.hpp"
#include "FXStateManager.hpp"
#include "SpriteRenderSystem.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/gl.h>




class GameRenderSystem {
private:
    SpriteRenderSystem sprite_renderer_;

    learn::PostprocessRenderer pp_renderer_;
    learn::PostprocessDoubleBuffer ppdb_;

    learn::ShaderProgram pp_shake_;
    learn::ShaderProgram pp_chaos_;
    learn::ShaderProgram pp_confuse_;

public:
    GameRenderSystem(gl::GLsizei width, gl::GLsizei height);

    void draw(entt::registry& registry, const FXStateManager& fx_manager);

    void reset_size(gl::GLsizei width, gl::GLsizei height) {
        ppdb_.reset_size(width, height);
    }
};



