#include "GameRenderSystem.hpp"
#include "ShaderBuilder.hpp"
#include "SpriteRenderSystem.hpp"
#include "Canvas.hpp"
#include "GlobalsUtil.hpp"
#include <glm/glm.hpp>
#include <entt/entt.hpp>


GameRenderSystem::GameRenderSystem(gl::GLsizei width, gl::GLsizei height)
    : sprite_renderer_{
        glm::ortho(
            canvas.bound_left(), canvas.bound_right(),
            canvas.bound_bottom(), canvas.bound_top(),
            zdepth::near, zdepth::far
        )
    }
    , ppdb_{ width, height }
    , pp_shake_{
        learn::ShaderBuilder()
            .load_vert("src/breakout2/shaders/pp_shake.vert")
            .load_frag("src/breakout/shaders/pp_kernel_blur.frag")
            .get()
    }
    , pp_chaos_{
        learn::ShaderBuilder()
            .load_vert("src/breakout2/shaders/pp_chaos.vert")
            .load_frag("src/breakout/shaders/pp_kernel_edge.frag")
            .get()
    }
    , pp_confuse_{
        learn::ShaderBuilder()
            .load_vert("src/breakout2/shaders/pp_confuse.vert")
            .load_frag("src/breakout/shaders/pp_invert.frag")
            .get()
    }
{}




void GameRenderSystem::draw(entt::registry& registry, const FXStateManager& fx_manager) {
    using namespace gl;

    // Scene rendering.
    ppdb_.draw_and_swap([&, this] {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        sprite_renderer_.draw_sprites(registry);
    });

    // Post-effects.
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

    // Blitting to the screen backbuffer.
    auto [w, h] = learn::globals::window_size.size();

    learn::BoundFramebuffer::unbind_as(GL_DRAW_FRAMEBUFFER);
    ppdb_.front().framebuffer()
        .bind_as(GL_READ_FRAMEBUFFER)
        .blit(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST)
        .unbind();

}
