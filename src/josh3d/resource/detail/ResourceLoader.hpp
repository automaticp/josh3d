#pragma once
#include "../ResourceLoader.hpp"


namespace josh {


class ResourceLoader::Access {
public:
    auto& resource_database()  noexcept { return self_.resource_database_;  }
    auto& resource_registry()  noexcept { return self_.resource_registry_;  }
    auto& thread_pool()        noexcept { return self_.thread_pool_;        }
    auto& offscreen_context()  noexcept { return self_.offscreen_context_;  }
    auto& completion_context() noexcept { return self_.completion_context_; }
    auto& task_counter()       noexcept { return self_.task_counter_;       }
    auto& local_context()      noexcept { return self_.local_context_;      }
    auto& scene_registry()     noexcept { return self_.scene_registry_;     }
private:
    friend ResourceLoader;
    Access(ResourceLoader& self) : self_{ self } {}
    ResourceLoader& self_;
};


} // namespace josh


namespace josh::detail {


[[nodiscard]]
auto load_scene_async(
    ResourceLoader::Access loader,
    UUID                   uuid,
    Handle                 dst_handle)
        -> Job<void>;


} // namespace josh::detail
