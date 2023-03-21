#pragma once
#include "GlobalsUtil.hpp"
#include "PostprocessDoubleBuffer.hpp"
#include "PostprocessRenderer.hpp"
#include "GLObjects.hpp"
#include "FXStateManager.hpp"
#include <glbinding/gl/gl.h>
#include <glbinding/gl/types.h>



class VFXRenderer {
private:
    learn::PostprocessRenderer pp_renderer_;
    learn::PostprocessDoubleBuffer ppdb_;

    learn::ShaderProgram pp_shake_;
    learn::ShaderProgram pp_chaos_;
    learn::ShaderProgram pp_confuse_;

public:
    VFXRenderer(gl::GLsizei width, gl::GLsizei height);

    void draw(auto&& scene_draw_function, const FXStateManager& fx_manager);

    void reset_size(gl::GLsizei width, gl::GLsizei height) { ppdb_.reset_size(width, height); }
};



inline void VFXRenderer::draw(auto&& scene_draw_fun, const FXStateManager& fx_manager) {
    using namespace gl;

    ppdb_.draw_and_swap(scene_draw_fun);

    if (fx_manager.is_active(FXType::confuse)) {
        ppdb_.draw_and_swap([this] {
            auto asp = pp_confuse_.use();
            pp_renderer_.draw(asp, ppdb_.front_target());
        });
    }

    if (fx_manager.is_active(FXType::chaos)) {
        ppdb_.draw_and_swap([this] {

            GLenum old_wrap;
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &old_wrap);

            ppdb_.front_target().bind()
                .set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT)
                .set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT)
                .and_then([this] {
                    auto asp = pp_chaos_.use();
                    asp.uniform("time", learn::globals::frame_timer.current<float>());
                    pp_renderer_.draw(asp, ppdb_.front_target());
                })
                .set_parameter(GL_TEXTURE_WRAP_S, old_wrap)
                .set_parameter(GL_TEXTURE_WRAP_T, old_wrap)
                .unbind();
        });
    }

    if (fx_manager.is_active(FXType::shake)) {
        ppdb_.draw_and_swap([this] {
            auto asp = pp_shake_.use();
            asp.uniform("time", learn::globals::frame_timer.current<float>());
            pp_renderer_.draw(asp, ppdb_.front_target());
        });
    }


    auto [w, h] = learn::globals::window_size.size();

    learn::BoundFramebuffer::unbind_as(GL_DRAW_FRAMEBUFFER);
    ppdb_.front().framebuffer()
        .bind_as(GL_READ_FRAMEBUFFER)
        .blit(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST)
        .unbind();

}
