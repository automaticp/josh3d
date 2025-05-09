#include "OffscreenContext.hpp"
#include "Future.hpp"
#include "ThreadName.hpp"
#include <exception>
#include <glfwpp/window.h>


namespace josh {


OffscreenContext::OffscreenContext(const glfw::Window& shared_with)
    : offscreen_thread_{
        [&, this](std::stop_token stoken) { // NOLINT
            glfw::WindowHints{
                .visible             = false,
                .contextVersionMajor = 4,
                .contextVersionMinor = 6,
                .openglProfile       = glfw::OpenGlProfile::Core,
            }.apply();

            glfw::Window window{ 1, 1, "Offscreen Context", nullptr, &shared_with };

            set_current_thread_name("offscreen ctx");
            startup_latch_.arrive_and_wait();

            offscreen_thread_loop(std::move(stoken), window);
        }
    }
{
    startup_latch_.arrive_and_wait();
}


void OffscreenContext::offscreen_thread_loop(
    std::stop_token stoken, // NOLINT
    glfw::Window&   window)
{
    while (!stoken.stop_requested()) {
        std::optional request = requests_.wait_and_pop(stoken);
        if (!request.has_value()) {
            break;
        }
        // Ensure the context is current before each invocation.
        // This is to "fool-proof" away from switching contexts in a task.
        //
        // TODO: This shouldn't be expensive, but is it really not?
        glfw::makeContextCurrent(window);
        try {
            request->task(window);
            set_result(std::move(request->promise));
        } catch (...) {
            set_exception(std::move(request->promise), std::current_exception());
        }
    }
}


auto OffscreenContext::emplace_request(Task task)
    -> Future<void>
{
    auto [future, promise] = make_future_promise_pair<void>();
    requests_.emplace(std::move(task), std::move(promise));
    return std::move(future);
}


} // namespace josh
