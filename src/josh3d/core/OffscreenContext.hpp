#pragma once
#include "CategoryCasts.hpp"
#include "Future.hpp"
#include "GLFenceSync.hpp"
#include "GLMutability.hpp"
#include "ThreadsafeQueue.hpp"
#include "UniqueFunction.hpp"
#include <concepts>
#include <latch>
#include <stop_token>
#include <thread>


namespace glfw { class Window; }


namespace josh {


class OffscreenContext
{
public:
    OffscreenContext(const glfw::Window& shared_with);

    template<typename FuncT>
        requires std::invocable<FuncT> or std::invocable<FuncT, glfw::Window&>
    auto emplace(FuncT&& func) -> Future<void>;

private:
    using Task = UniqueFunction<void(glfw::Window&)>;

    auto emplace_request(Task task)
        -> Future<void>;

    struct Request
    {
        Task          task;
        Promise<void> promise;
    };

    std::latch               startup_latch_{ 2 };
    ThreadsafeQueue<Request> requests_;
    std::jthread             offscreen_thread_;

    void offscreen_thread_loop(std::stop_token stoken, glfw::Window& window);
};


template<typename FuncT>
    requires std::invocable<FuncT> or std::invocable<FuncT, glfw::Window&>
auto OffscreenContext::emplace(FuncT&& func)
    -> Future<void>
{
    if constexpr (std::invocable<FuncT, glfw::Window&>)
        return emplace_request(FORWARD(func));
    else
        return emplace_request([func=FORWARD(func)](glfw::Window&) { func(); });
}


/*
Support of the `readyable` concept for FenceSync.

FIXME: Should I put this somewhere else?
*/
auto is_ready(std::convertible_to<RawFenceSync<GLConst>> auto&& fence)
    -> bool
{
    return decay_to_raw(fence).has_signaled();
}


} // namespace josh
