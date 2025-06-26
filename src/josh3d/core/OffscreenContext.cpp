#include "OffscreenContext.hpp"
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "Future.hpp"
#include "ThreadName.hpp"
#include "Tracy.hpp"
#include <glfwpp/window.h>
#include <exception>


namespace josh {


OffscreenContext::OffscreenContext(const glfw::Window& shared_with)
    : offscreen_thread_{
        [&, this](std::stop_token stoken) // NOLINT
        {
            glfw::WindowHints{
                .visible             = false,
                .contextVersionMajor = 4,
                .contextVersionMinor = 6,
                .openglProfile       = glfw::OpenGlProfile::Core,
            }.apply();

            glfw::Window window{ 1, 1, "Offscreen Context", nullptr, &shared_with };

            TracyGpuContext;
            TracyGpuContextName("offscreen ctx", 13);
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
    while (!stoken.stop_requested())
    {
        Optional request = requests_.wait_and_pop(stoken);
        if (!request.has_value())
            break;

        // Ensure the context is current before each invocation.
        // This is to "fool-proof" away from switching contexts in a task.
        //
        // TODO: This shouldn't be expensive, but is it really not?
        glfw::makeContextCurrent(window);
        try
        {
            request->task(window);
            set_result(MOVE(request->promise));
        }
        catch (...)
        {
            set_exception(MOVE(request->promise), std::current_exception());
        }
    }
}

auto OffscreenContext::emplace_request(Task task)
    -> Future<void>
{
    auto [future, promise] = make_future_promise_pair<void>();
    requests_.emplace(MOVE(task), MOVE(promise));
    return MOVE(future);
}


} // namespace josh
