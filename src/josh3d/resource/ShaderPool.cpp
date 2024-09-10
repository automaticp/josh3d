#include "ShaderPool.hpp"
#include "Components.hpp"
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GLProgram.hpp"
#include "GLShaders.hpp"
#include "ObjectLifecycle.hpp"
#include "ReadFile.hpp"
#include "SceneGraph.hpp"
#include "ShaderBuilder.hpp"
#include "ShaderSource.hpp"
#include "ThreadsafeQueue.hpp"
#include "UniqueFunction.hpp"
#include "Logging.hpp"
#include <entt/entity/fwd.hpp>
#include <cassert>
#include <latch>
#include <memory>
#include <sstream>
#include <stop_token>
#include <thread>
#include <utility>
#include <ranges>
#include <variant>


// There are better ways to do this, I'm sure.
#ifdef __linux__
    #define JOSH3D_SHADER_POOL_LINUX
#else
    #define JOSH3D_SHADER_POOL_COMMON
#endif


#ifdef JOSH3D_SHADER_POOL_LINUX
    #include <sys/inotify.h>
    #include <linux/limits.h>
    #include <unistd.h>
    #include <csignal>
    #include <cerrno>
#endif



namespace josh {
namespace {


// TODO: Should be general Utility.
struct Immovable {
    constexpr Immovable()                            = default;
    constexpr Immovable(const Immovable&)            = delete;
    constexpr Immovable(Immovable&&)                 = delete;
    constexpr Immovable& operator=(const Immovable&) = delete;
    constexpr Immovable& operator=(Immovable&&)      = delete;
};


using ProgramID       = entt::entity;
using FileID          = entt::entity;
using ProgramOrFileID = entt::entity;
using Registry        = entt::registry;
using Handle          = entt::handle;
using ConstHandle     = entt::const_handle;


#ifdef JOSH3D_SHADER_POOL_LINUX


class ShaderWatcherLinux {
public:
    ShaderWatcherLinux();

    void watch(const FileID& id, const File& file);
    auto get_next_modified() -> std::optional<FileID>;
    void stop_watching(const FileID& id);

private:

    class INotifyInstance : private Immovable {
    public:
        // TODO: Should throw on failure.
        INotifyInstance() : fd_{ inotify_init() } {}
        ~INotifyInstance() noexcept { close(fd_); }
        auto fd() const noexcept -> int { return fd_; }
    private:
        int fd_;
    };

    using WD = int; // Watch Descriptor.

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




extern "C" void dummy_sigh(int) {}




void ShaderWatcherLinux::read_thread_loop(std::stop_token stoken) { // NOLINT(performance-*)

    // Stop SIGINT from actually interrupting the whole process,
    // if sent to this thread. We use it to cancel out of the read() call instead.
    //
    // signal() and pthread_sigmask() do not work, unfortunately.
    // Resetting the `sigaction` without SA_RESTART seems to work.
    //
    // NOTE: I have a fairly low confidence in what I'm doing with Linux syscalls.
    struct sigaction s{};
    s.sa_handler = dummy_sigh;
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



#endif // JOSH3D_SHADER_POOL_LINUX


} // namespace



/*
Mini-ECS setup here.

3 types of entities: Programs, Primary Files and Secondary (included) Files.

Typedefs of ProgramID and FileID are used to
better differentiate between them in code.

ProgramID:
    - A parent of a set of "primary" files;
    - Can be MarkedForReload, this destroys the descendants
        of the primary files, and reloads primary files anew.
        This is because includes can be removed and added in the
        process.
    - Are hashed with ProgramName and stored in a side pool
        for "amortized" lookup on repeated get() calls on a pool;
    - Optionally contains ProgramDefines to indicate program-wide
        macro definitions.

FileID:
    - Primary files have ShaderTarget, others don't.
    - Primary files are both AsChild and can be AsParent;
    - Secondary (included) files are only AsChild and are a flat list;
    - Have `File` to indicate the file they came from;




*/


struct MarkedForReload {};



struct ProgramName  {
    std::string str;

    static auto from_filenames(const ProgramFiles& files)
        -> ProgramName
    {
        return from_filenames_and_defines(files, {});
    }

    static auto from_filenames_and_defines(
        const ProgramFiles&   files,
        const ProgramDefines& defines)
            -> ProgramName
    {
        // TODO: Could probably just resize() a string and append,
        // given that we know lengths of all strings ahead of time.
        std::stringstream ss;

        #define APPEND_PATH(type) \
            if (files.type) { ss << "##" #type ":" << files.type->path(); }

        APPEND_PATH(vert);
        APPEND_PATH(tesc);
        APPEND_PATH(tese);
        APPEND_PATH(geom);
        APPEND_PATH(frag);
        APPEND_PATH(comp);

        #undef APPEND_PATH

        for (const auto& define : defines.values) {
            ss << define;
        }

        return { std::move(ss).str() };
    }

    constexpr auto operator<=>(const ProgramName&) const = default;

};
} // namespace josh
template<>
struct std::hash<josh::ProgramName> {
    auto operator()(const josh::ProgramName& name) const noexcept {
        return std::hash<std::string>{}(name.str);
    }
};
namespace josh {




class ShaderPoolImpl {
public:
    friend ShaderToken;
    ShaderPoolImpl();

    auto get(const ProgramFiles& program_files) -> ShaderToken;
    auto get(const ProgramFiles& program_files, const ProgramDefines& defines) -> ShaderToken;

    bool supports_hot_reload() const noexcept;
    void hot_reload();
    void force_reload();

private:
#ifdef JOSH3D_SHADER_POOL_LINUX
    ShaderWatcherLinux watcher_;
#endif
    void sweep_reload_marked();
    Registry                                   registry_;
    std::unordered_map<ProgramName, ProgramID> program_map_;
};




ShaderPoolImpl::ShaderPoolImpl() = default;


namespace {


// Type-per-shader was a mistake.
using AnyShader = std::variant<
    UniqueVertexShader,
    UniqueTessControlShader,
    UniqueTessEvaluationShader,
    UniqueGeometryShader,
    UniqueFragmentShader,
    UniqueComputeShader
>;


auto make_any_shader(ShaderTarget target)
    -> AnyShader
{
    switch (target) {
        using enum ShaderTarget;
        case VertexShader:         return UniqueVertexShader();
        case TessControlShader:    return UniqueTessControlShader();
        case TessEvaluationShader: return UniqueTessEvaluationShader();
        case GeometryShader:       return UniqueGeometryShader();
        case FragmentShader:       return UniqueFragmentShader();
        case ComputeShader:        return UniqueComputeShader();
        default: assert(false);
    }
}


auto load_and_attach_primary(
    Handle program_handle, // Must have clear UniqueProgram, can have ProgramDefines.
    Handle primary_handle) // Must have File and ShaderTarget, but no children.
{
    auto& registry = *program_handle.registry();

    assert(has_component<UniqueProgram>(program_handle));
    assert(has_component<File>(primary_handle) && has_component<ShaderTarget>(primary_handle));

    const auto& file   = primary_handle.get<File>();
    const auto& target = primary_handle.get<ShaderTarget>();

    auto source = ShaderSource(read_file(file));

    const auto& path = file.path();


    // Recipe to resolve includes:
    //
    // 1. Find include directive and extract relative path.
    // 2. Get canonical path of the include, handle errors.
    // 3. Check if it has already been included, if it has:
    //   - Replace the #include with an empty string.
    //   - Repeat from (1).
    // 4. Try to read file of the include, handle errors.
    // 5. Replace the #include with the contents of the new file.
    // 6. Add canonical path of the new include to the "included set".
    // 7. Repeat from (1) unitl no more #includes are found.
    // 8. Create a new FileID to represent each included file,
    //    emplace File into it and attach it to primary.

    thread_local std::unordered_set<File> included;
    included.clear();

    auto trim1 = [](std::string_view sv) {
        sv.remove_prefix(1);
        sv.remove_suffix(1);
        return sv;
    };

    Path parent_dir = path.parent_path();

    while (auto include_info = source.find_include_directive()) {
        std::string_view trimmed = trim1(include_info->include_arg);

        Path relative_path  = { trimmed };
        File canonical_file = File(std::filesystem::canonical(parent_dir / relative_path)); // TODO: Can fail.

        if (included.contains(canonical_file)) {
            // Just erase the #include line.
            ShaderSource empty_contents{{}};

            source.replace_include_line_with_contents(
                include_info->line_begin,
                include_info->line_end,
                empty_contents
            );

        } else {
            // TODO: Can fail.
            ShaderSource included_contents{ read_file(canonical_file) };

            source.replace_include_line_with_contents(
                include_info->line_begin,
                include_info->line_end,
                included_contents
            );

            included.emplace(canonical_file);
        }
    }


    // Okay, we pick-up the list of included only after processing the whole file.

    for (const File& included_file : included) {
        Handle new_secondary{ registry, registry.create() };
        attach_to_parent(new_secondary, primary_handle);
        new_secondary.emplace<File>(included_file);
    }


    // Now we need to define all the defines that we had prepared.

    if (const auto* defines = program_handle.try_get<ProgramDefines>()) {

        for (const auto& define : defines->values) {

            const bool was_found =
                // TODO: "#version" is not exact. Could be "  #   version" and friends.
                source.find_and_insert_as_next_line("#version", define);

            // If the "#version" is not found, just drop it at the first line.
            // This will likely fail to compile anyway...
            if (!was_found) {
                source.insert_line_after(0, define);
            }

        }

    }

    // Okay, cool, now we get to actually compile the shader from primary source.

    auto&     program = program_handle.get<UniqueProgram>();
    AnyShader shader  = make_any_shader(target);

    auto compile_and_attach = [&](auto& shader_obj) {
        shader_obj->set_source(source.text());
        shader_obj->compile();
        if (!shader_obj->has_compiled_successfully()) {
            throw error::ShaderCompilationFailure(
                path.string() + '\n' + shader_obj->get_info_log(),
                target
            );
        }
        program->attach_shader(shader_obj);
    };

    visit(compile_and_attach, shader);

    // NOTE: `shader` is discarded here.
}


auto create_and_attach_primary(
    Handle       program_handle, // Must have UniqueProgram, can have ProgramDefines
    const File&  file,
    ShaderTarget target)
        -> Handle
{
    auto& registry = *program_handle.registry();
    Handle new_primary{ registry, registry.create() };

    // Attach asap, so that on exception we could just destroy_subtree().
    attach_to_parent(new_primary, program_handle.entity());

    new_primary.emplace<ShaderTarget>(target);
    new_primary.emplace<File>(file);

    load_and_attach_primary(program_handle, new_primary);

    return new_primary;
}


} // namespace




auto ShaderPoolImpl::get(const ProgramFiles& files)
    -> ShaderToken
{
    return get(files, {});
}


auto ShaderPoolImpl::get(const ProgramFiles& files, const ProgramDefines& defines)
    -> ShaderToken
{
    ProgramName program_name = ProgramName::from_filenames_and_defines(files, defines);

    // Get from cache.
    auto it = program_map_.find(program_name);
    if (it != program_map_.end()) {
        assert(registry_.valid(it->second));
        return ShaderToken(it->second, this);
    }

    // Or load a new one...
    //
    // We reload each file from scratch, even if it was already loaded in another program.
    // Each dependency tree formed by each program is considered separate this way.
    // This is easier to deal with in most cases.
    //
    // The consequence of that is that each actual file can have multiple FileID referring
    // to id, on the grounds that each FileID belongs to a separate subtree.
    //
    // This is reflected in the ShaderWatcher.

    /*
    ProgramID:
        - A parent of a set of "primary" files;
        - Can be MarkedForReload, this destroys the descendants
          of the primary files, and reloads primary files anew.
          This is because includes can be removed and added in the
          process.
        - Are hashed with ProgramName and stored in a side pool
          for "amortized" lookup on repeated get() calls on a pool;
        - Optionally contains ProgramDefines to indicate program-wide
          macro definitions.

    FileID:
        - Primary files have ShaderTarget, others don't.
        - Primary files are both AsChild and can be AsParent;
        - Secondary (included) files are only AsChild and are a flat list;
        - Have File to indicate the file they came from;

    */

    auto& registry = registry_;
    Handle new_program{ registry, registry.create() };

    try {
        const auto& name = new_program.emplace<ProgramName>(std::move(program_name));
        new_program.emplace<ProgramDefines>(defines);
        auto& program = new_program.emplace<UniqueProgram>();

        if (files.vert) { create_and_attach_primary(new_program, *files.vert, ShaderTarget::VertexShader        ); }
        if (files.tesc) { create_and_attach_primary(new_program, *files.tesc, ShaderTarget::TessControlShader   ); }
        if (files.tese) { create_and_attach_primary(new_program, *files.tese, ShaderTarget::TessEvaluationShader); }
        if (files.geom) { create_and_attach_primary(new_program, *files.geom, ShaderTarget::GeometryShader      ); }
        if (files.frag) { create_and_attach_primary(new_program, *files.frag, ShaderTarget::FragmentShader      ); }
        if (files.comp) { create_and_attach_primary(new_program, *files.comp, ShaderTarget::ComputeShader       ); }

        program->link();

        if (!program->has_linked_successfully()) {
            throw error::ProgramLinkingFailure(program->get_info_log());
        }

        program_map_.insert_or_assign(name, new_program.entity());

#ifdef JOSH3D_SHADER_POOL_LINUX
        // Install watches on all descendants.
        traverse_descendants_preorder(new_program, [this](Handle child) {
            assert(has_component<File>(child));
            watcher_.watch(child.entity(), child.get<File>()); // I'm a watcher.
        });

#endif

    } catch (...) {
        destroy_subtree(new_program);
        throw;
    }

    return ShaderToken(new_program.entity(), this);
}


bool ShaderPoolImpl::supports_hot_reload() const noexcept {
#ifdef JOSH3D_SHADER_POOL_LINUX
    return true;
#else
    return false;
#endif
}



void ShaderPoolImpl::sweep_reload_marked() {

    // Sweep-reload each independently.
    // We just drop the whole program and reload it again. This is much simpler.
    for (const ProgramID program : registry_.view<MarkedForReload>()) {
        const Handle program_handle{ registry_, program };

        // We don't need to reset everything here.
        //
        // What stays:
        //  - ProgramName, ProgramDefines
        //  - List of Primary Files and their Targets
        //
        // What gets reset:
        //  - UniqueProgram
        //  - All secondary (include) files are destroyed
        //  - All watches of secondaries are destroyed too
        //

        for (const Handle primary : view_child_handles(program_handle)) {
            for (const Handle secondary : view_child_handles(primary)) {
#ifdef JOSH3D_SHADER_POOL_LINUX
                watcher_.stop_watching(secondary.entity());
#endif
                mark_for_detachment(secondary);
                mark_for_destruction(secondary);
            }
        }
    }

    sweep_marked_for_detachment(registry_);
    sweep_marked_for_destruction(registry_);

    // Iterate again to now reload all the shaders.
    for (const ProgramID program : registry_.view<MarkedForReload>()) {
        const Handle program_handle{ registry_, program };

        // Preserve the old program in case something goes wrong.
        // This time, we don't want to terminate the application,
        // so we'll just log the error. NOTE: This is a bit messy.
        UniqueProgram  old_program = std::move(program_handle.get<UniqueProgram>());
        UniqueProgram& new_program = program_handle.get<UniqueProgram>();
        new_program = {};


        try {

            for (const Handle primary : view_child_handles(program_handle)) {
                load_and_attach_primary(program_handle, primary);
#ifdef JOSH3D_SHADER_POOL_LINUX
                for (const Handle secondary : view_child_handles(primary)) {
                    watcher_.watch(secondary.entity(), secondary.get<File>());
                }
#endif
            }

            new_program->link();

            if (!new_program->has_linked_successfully()) {
                throw error::ProgramLinkingFailure(new_program->get_info_log());
            }

        } catch (const std::exception& e) {
            // Restore the old program.
            program_handle.get<UniqueProgram>() = std::move(old_program);
            globals::logstream << "[SHADER RELOAD FAILED]: " << e.what() << '\n';
        }

        // TODO: Need a "Watched" component, so that nodes could automatically
        // unwatch themselves on destruction.
    }

    registry_.clear<MarkedForReload>();

    // FIXME: If the file is modified during reloads, we're screwed a bit.
    // We need to load sources first, then from that snapshot recompile in batch.
    //
    // The problem is that in that case, we need to keep a local copy of the filesystem
    // contents, so that the shader includes can be resolved from the local snapshot of sources.
    //
    // Aaand now we're reinventing GL_ARB_shading_language_include...

}




void ShaderPoolImpl::hot_reload() {
#ifdef JOSH3D_SHADER_POOL_LINUX

    // Mark roots of each tree for reload.
    while (const std::optional<FileID> modified = watcher_.get_next_modified()) {
        const Handle handle{ registry_, *modified };

        auto root = get_root_handle(handle);

        // Roots are always Programs.
        assert(has_component<UniqueProgram>(root));

        root.emplace_or_replace<MarkedForReload>();
    };

    // Then sweep.
    sweep_reload_marked();

#endif
}


void ShaderPoolImpl::force_reload() {

    // Mark all roots for reload.
    for (const auto program : registry_.view<UniqueProgram>()) {
        const Handle handle{ registry_, program };
        handle.emplace_or_replace<MarkedForReload>();
    }

    // Then sweep.
    sweep_reload_marked();
}





auto ShaderToken::get() const noexcept
    -> RawProgram<GLConst>
{
    return pool_->registry_.get<UniqueProgram>(id_);
}


auto ShaderToken::get() noexcept
    -> RawProgram<GLMutable>
{
    return pool_->registry_.get<UniqueProgram>(id_);
}




// Boring boilerplate for PIMPL.


ShaderPool::ShaderPool() : pimpl_{ std::make_unique<ShaderPoolImpl>() } {}
ShaderPool::~ShaderPool() = default;


auto ShaderPool::get(const ProgramFiles& program_files)
    -> ShaderToken
{
    return pimpl_->get(program_files);
}


auto ShaderPool::get(
    const ProgramFiles&   program_files,
    const ProgramDefines& defines)
        -> ShaderToken
{
    return pimpl_->get(program_files, defines);
}


bool ShaderPool::supports_hot_reload() const noexcept {
    return pimpl_->supports_hot_reload();
}


void ShaderPool::hot_reload() {
    if (!supports_hot_reload()) {
        throw std::logic_error("Hot-reloading is not supported.");
    }
    pimpl_->hot_reload();
}


void ShaderPool::force_reload() {
    pimpl_->force_reload();
}






thread_local std::optional<ShaderPool> thread_local_shader_pool_;


auto shader_pool()
    -> ShaderPool&
{
    return thread_local_shader_pool_.value();
}


void init_thread_local_shader_pool() {
    thread_local_shader_pool_.emplace();
}


void clear_thread_local_shader_pool() {
    thread_local_shader_pool_.reset();
}



} // namespace josh
