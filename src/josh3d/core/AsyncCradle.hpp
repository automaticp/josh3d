#pragma once
#include "CompletionContext.hpp"
#include "LocalContext.hpp"
#include "OffscreenContext.hpp"
#include "Scalars.hpp"
#include "Semantics.hpp"
#include "TaskCounterGuard.hpp"
#include "ThreadPool.hpp"


namespace glfw { class Window; }


namespace josh {

struct AsyncCradleRef;

/*
A collection of async contexts and helpers used widely across all job-based systems.
*/
struct AsyncCradle
    : private Immovable<AsyncCradle>
{
    ThreadPool        task_pool;          // Primary thread pool for compute work.
    ThreadPool        loading_pool;       // Separate thread pool for importing/loading/unpacking jobs.
    CompletionContext completion_context; // Spinning context for awaiting jobs. Mostly redundant.
    OffscreenContext  offscreen_context;  // Offscreen GPU context for offloading GPU tasks.
    TaskCounterGuard  task_counter;       // Task counter used for detecting when all tasks are complete.
    LocalContext      local_context;      // Main-thread context run during per-frame update. Must be last.

    AsyncCradle(
        usize               task_pool_size,
        usize               loading_pool_size,
        const glfw::Window& main_window
    )
        : task_pool         (task_pool_size, "task pool")
        , loading_pool      (loading_pool_size, "load pool")
        , completion_context()
        , offscreen_context (main_window)
        , task_counter      ()
        , local_context     (task_counter)
    {}

    operator AsyncCradleRef() noexcept;
};

/*
HMM: Uh, why does this exist at all? Why not pass AsyncCradle& around?
*/
struct AsyncCradleRef
{
    ThreadPool&        task_pool;
    ThreadPool&        loading_pool;
    CompletionContext& completion_context;
    OffscreenContext&  offscreen_context;
    TaskCounterGuard&  task_counter;
    LocalContext&      local_context;
};

inline AsyncCradle::operator AsyncCradleRef() noexcept
{
    return {
        .task_pool          = task_pool,
        .loading_pool       = loading_pool,
        .completion_context = completion_context,
        .offscreen_context  = offscreen_context,
        .task_counter       = task_counter,
        .local_context      = local_context,
    };
}


} // namespace josh
