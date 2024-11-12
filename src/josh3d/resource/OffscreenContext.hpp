#pragma once
#include "Future.hpp"
#include "ThreadsafeQueue.hpp"
#include "UniqueFunction.hpp"
#include <latch>
#include <stop_token>
#include <thread>


namespace glfw { class Window; }


namespace josh {


class OffscreenContext {
public:
    using Task = UniqueFunction<void(glfw::Window&)>;

    OffscreenContext(const glfw::Window& shared_with);

    auto emplace(Task task) -> Future<void>;

private:
    struct Request {
        Task          task;
        Promise<void> promise;
    };
    std::latch               startup_latch_{ 2 };
    ThreadsafeQueue<Request> requests_;
    std::jthread             offscreen_thread_;

    void offscreen_thread_loop(std::stop_token stoken, glfw::Window& window);
};


} // namespace josh
