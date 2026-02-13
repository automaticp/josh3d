#include "ShaderWatcher.hpp"
#include "Errors.hpp"
#include "async/ThreadAttributes.hpp"
#include <ranges>


#ifdef JOSH3D_SHADER_WATCHER_LINUX
extern "C" void josh_dummy_signal_handler(int) {}
#endif


namespace josh::detail {


#ifdef JOSH3D_SHADER_WATCHER_LINUX


ShaderWatcherLinux::INotifyInstance::INotifyInstance()
    : fd_{ inotify_init() }
{
    if (fd_ == -1) {
        throw RuntimeError("inotify_init() failed when trying to setup ShaderWatcher.");
    }
}


ShaderWatcherLinux::INotifyInstance::~INotifyInstance() noexcept {
    if (fd_ != -1) {
        close(fd_);
    }
}




ShaderWatcherLinux::ShaderWatcherLinux()
    : read_thread_{
        [this](std::stop_token stoken) { read_thread_loop(std::move(stoken)); }
    }
{
    pthread_interrupt_.emplace(
        read_thread_.get_stop_token(),
        [this]{ pthread_kill(read_thread_.native_handle(), SIGINT); }
    );
    startup_latch_.arrive_and_wait();
}




void ShaderWatcherLinux::read_thread_loop(std::stop_token stoken) { // NOLINT(performance-*)
    set_current_thread_name("shader watcher");

    // Stop SIGINT from actually interrupting the whole process,
    // if sent to this thread. We use it to cancel out of the read() call instead.
    //
    // signal() and pthread_sigmask() do not work, unfortunately.
    // Resetting the `sigaction` without SA_RESTART seems to work.
    //
    // NOTE: I have a fairly low confidence in what I'm doing with Linux syscalls.
    struct sigaction s{};
    s.sa_handler = josh_dummy_signal_handler;
    sigaction(SIGINT, &s, nullptr);

    startup_latch_.arrive_and_wait(); // Wait for stop callback to be installed.

    constexpr auto buf_align = alignof(inotify_event_header);
    constexpr auto buf_size  = sizeof(inotify_event_header) + NAME_MAX + 1;

    alignas(buf_align) char buf[buf_size];

    while (!stoken.stop_requested()) {

        // Blocks here until a some watched files are modified.
        // Will be interrupted from the stop_callback with SIGINT.
        auto read_result =
            read(inotify_.fd(), buf, buf_size);

        if (stoken.stop_requested()) {
            assert((read_result == 0 || read_result == -1) && errno == EINTR);
            break;
        }

        // I LOVE C APIs.
        const char* current_event_ptr = buf;
        const char* buf_end           = buf + buf_size;

        auto bytes_to_read = read_result;

        while (bytes_to_read > 0 && current_event_ptr != buf_end) {

            const auto* event =
                reinterpret_cast<const inotify_event_header*>(current_event_ptr);

            if (event->mask & IN_MODIFY) {
                wd_events_.push(event->wd);
            }

            const auto bytes_read = sizeof(inotify_event_header) + event->len;

            bytes_to_read     -= ssize_t(bytes_read);
            current_event_ptr += bytes_read;
        }

    }
}




void ShaderWatcherLinux::watch(const FileID& id, const File& file) {
    const WD wd = inotify_add_watch(inotify_.fd(), file.path().c_str(), IN_MODIFY);
    wd2id_.emplace(wd, id);
    id2wd_.emplace(id, wd);
}




auto ShaderWatcherLinux::get_next_modified()
    -> std::optional<FileID>
{
    // Flush pending WD events. Resolve each WD to all subscribed FileIDs.
    while (auto wd_event = wd_events_.try_pop()) {
        const WD wd = *wd_event;
        auto [beg, end] = wd2id_.equal_range(wd);
        for (const FileID file_id : std::ranges::subrange(beg, end) | std::views::values) {
            file_id_events_.push(file_id);
        }
    }

    // Pop from the FileID events queue.
    if (!file_id_events_.empty()) {
        FileID id = file_id_events_.front();
        file_id_events_.pop();
        return id;
    } else {
        return std::nullopt;
    }
}




void ShaderWatcherLinux::stop_watching(const FileID& id) {
    const WD wd = id2wd_.at(id);

    if (wd2id_.bucket_size(wd2id_.bucket(wd)) == 1) {
        // This would be the last one removed.
        // Actually remove everything for real this time.
        auto error [[maybe_unused]] =
            inotify_rm_watch(inotify_.fd(), wd);
        assert(error == 0); // Errors only on API misuse.
        id2wd_.erase(id);
    }

    auto [beg, end] = wd2id_.equal_range(wd);
    auto it = beg;
    for (; it != end; ++it) {
        if (it->second == id) {
            wd2id_.erase(it);
            break;
        }
    }
    assert(it != end && "We somehow haven't erased anything.");
}




#else


// No-op stubs for a fallback.
void ShaderWatcherFallback::watch(const FileID&, const File&) {}
auto ShaderWatcherFallback::get_next_modified() -> std::optional<FileID> { return std::nullopt; }
void ShaderWatcherFallback::stop_watching(const FileID&) {}




#endif // JOSH3D_SHADER_WATCHER_*




} // namespace josh::detail

