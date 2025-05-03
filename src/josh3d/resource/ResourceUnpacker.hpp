#pragma once
#include "CompletionContext.hpp"
#include "LocalContext.hpp"
#include "OffscreenContext.hpp"
#include "ResourceRegistry.hpp"
#include "TaskCounterGuard.hpp"
#include "ThreadPool.hpp"


namespace josh {


/*
Unpacking is the process of converting the intermediate resource
representation into its final "consumable" form for the target
destination.

The destination of unpacking could be any system that needs to
work on resulting data, for example the scene's mesh and material
components, skeleton/animation storage of the animation system, etc.

Unpacking never loads data from disk directly, and instead retrieves
all the data through the ResourceRegistry, which is responsible for
loading, caching and evicting actual resource data.
*/
class ResourceUnpacker {
public:
    ResourceUnpacker(
        ResourceRegistry&  resource_registry,
        ThreadPool&        thread_pool,
        OffscreenContext&  offscreen_context,
        CompletionContext& completion_context
    )
        : resource_registry_ { resource_registry  }
        , thread_pool_       { thread_pool        }
        , offscreen_context_ { offscreen_context  }
        , completion_context_{ completion_context }
    {}

    void update();

    class Context;

private:
    ResourceRegistry&  resource_registry_;
    ThreadPool&        thread_pool_;
    OffscreenContext&  offscreen_context_;
    CompletionContext& completion_context_;
    TaskCounterGuard   task_counter_;
    LocalContext       local_context_{ task_counter_ };
};


class ResourceUnpacker::Context {
public:
    auto& resource_registry()  noexcept { return self_.resource_registry_;  }
    auto& thread_pool()        noexcept { return self_.thread_pool_;        }
    auto& offscreen_context()  noexcept { return self_.offscreen_context_;  }
    auto& completion_context() noexcept { return self_.completion_context_; }
    auto& task_counter()       noexcept { return self_.task_counter_;       }
    auto& local_context()      noexcept { return self_.local_context_;      }
private:
    friend ResourceUnpacker;
    Context(ResourceUnpacker& self) : self_{ self } {}
    ResourceUnpacker& self_;
};



} // namespace josh
