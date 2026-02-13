#pragma once
#include "Filesystem.hpp"
#include "Semantics.hpp"
#include "async/ThreadsafeQueue.hpp"
#include "UniqueFunction.hpp"
#include <unordered_map>
#include <cstdint>
#include <stop_token>
#include <latch>
#include <thread>


// There are better ways to do this, I'm sure.
#ifdef __linux__
    #define JOSH3D_SHADER_WATCHER_LINUX
#else
    #define JOSH3D_SHADER_WATCHER_COMMON
#endif


#ifdef JOSH3D_SHADER_WATCHER_LINUX
    #include <sys/inotify.h>
    #include <linux/limits.h>
    #include <unistd.h>
    #include <csignal>
    #include <cerrno>
#endif


namespace josh::detail {


#ifdef JOSH3D_SHADER_WATCHER_LINUX


class ShaderWatcherLinux {
public:
    static constexpr bool actually_works = true;
    using FileID = uint32_t; // Some unique file identifier.

    ShaderWatcherLinux();

    void watch(const FileID& id, const File& file);
    auto get_next_modified() -> std::optional<FileID>;
    void stop_watching(const FileID& id);


private:
    using WD = int; // Watch Descriptor.

    class INotifyInstance : private Immovable<INotifyInstance> {
    public:
        INotifyInstance();
        ~INotifyInstance() noexcept;
        auto fd() const noexcept -> int { return fd_; }
    private:
        int fd_;
    };

    // Did something go wrong in the design of stop_callback?
    // I can't declare a type and use a lambda, and I can't
    // keep it alive longer that the thread because it needs
    // to be intialized with stop_token before the thread even starts?
    // This was, somehow, not one of the use-cases considered?
    using scb_type = std::stop_callback<UniqueFunction<void()>>;
    std::optional<scb_type> pthread_interrupt_;
    INotifyInstance         inotify_;
    std::latch              startup_latch_{ 2 };
    std::jthread            read_thread_;
    ThreadsafeQueue<WD>     wd_events_;
    void read_thread_loop(std::stop_token stoken);

    // Each WD can be reused by multiple "files".
    // Because each file has an independent id in each program tree.
    std::unordered_multimap<WD, FileID> wd2id_;
    std::unordered_map     <FileID, WD> id2wd_;

    std::queue<FileID> file_id_events_; // After converting one-to-many WD->FileID.

    struct inotify_event_header {
        WD       wd;     // Watch descriptor.
        uint32_t mask;   // Watch mask.
        uint32_t cookie; // Cookie to synchronize two events.
        uint32_t len;    // Length (including NULs) of name.
        // char name[];  // FAM. Ignored. We skip over it.
    };

};


#else


class ShaderWatcherFallback {
public:
    static constexpr bool actually_works = false;
    void watch(const FileID& id, const File& file);
    auto get_next_modified() -> std::optional<FileID>;
    void stop_watching(const FileID& id);
};


#endif // JOSH3D_SHADER_WATCHER_*


} // namespace josh::detail
