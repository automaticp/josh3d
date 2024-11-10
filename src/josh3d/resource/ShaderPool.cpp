#include "ShaderPool.hpp"
#include "Components.hpp"
#include "Filesystem.hpp"
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GLProgram.hpp"
#include "GLShaders.hpp"
#include "ObjectLifecycle.hpp"
#include "ReadFile.hpp"
#include "SceneGraph.hpp"
#include "ShaderBuilder.hpp"
#include "ShaderSource.hpp"
#include "Logging.hpp"
#include "detail/ShaderWatcher.hpp"
#include <entt/entity/fwd.hpp>
#include <cassert>
#include <exception>
#include <memory>
#include <optional>
#include <sstream>
#include <utility>
#include <variant>


namespace josh {


#ifdef JOSH3D_SHADER_WATCHER_LINUX
    using ShaderWatcher = detail::ShaderWatcherLinux;
#else
    using ShaderWatcher = detail::ShaderWatcherFallback;
#endif


/*
Mini-ECS setup here.

3 types of entities: Programs, Primary Files and Secondary (included) Files.
Typedefs of entt::entity are used to better differentiate between them in code.

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

PrimaryID:
    - Have ShaderTarget to identify the stage. One file per-stage,
        to keep things simple. Should not limit usability much.
    - Have `File` to indicate the file they came from.
    - Are AsChild and can be AsParent to a full recursive set of
        their includes.

SecondaryID:
    - Have `File` to indicate the file they came from.
    - *Only* AsChild and together constitute a flattened
        list of includes. This is because we don't care
        about partially reloading programs. If *any* of
        the files change, we just reload all the primary
        files anyway with all their includes.


NOTE: I think the only reason I went with using the ECS here
is because it automatically provides a stable "identifier"
for each file to pass to the Watcher API and expose in the ShaderToken.

Otherwise, this ends up being somewhat fragile, as we have
to maintain strict invariants in a system that was not
build for that (ECS).
*/


using ProgramID       = entt::entity;
using PrimaryID       = entt::entity;
using SecondaryID     = entt::entity;
using FileID          = entt::entity; // Any file, primary or secondary.
using ProgramOrFileID = entt::entity;
using Registry        = entt::registry;
using Handle          = entt::handle;
using ConstHandle     = entt::const_handle;


struct MarkedForReload {};


/*
Used to automatically watch/unwatch files when this component is created/destroyed.
The on_construct()/on_destroy() callbacks are installed in the ShaderPoolImpl constructor.
*/
struct WatchedFile {
    ShaderWatcher* watcher;
};


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
    ShaderWatcher                              watcher_; // I'm a watcher.
    Registry                                   registry_;
    std::unordered_map<ProgramName, ProgramID> program_map_;
    void sweep_reload_marked();
};




static void start_watching(Registry& registry, FileID entity) {
    const ConstHandle handle{ registry, entity };
    assert(has_component<File>(handle));
    const File& file = handle.get<File>();
    handle.get<WatchedFile>().watcher->watch(to_integral(entity), file);
}


static void stop_watching(Registry& registry, FileID entity) {
    const ConstHandle handle{ registry, entity };
    handle.get<WatchedFile>().watcher->stop_watching(to_integral(entity));
}


ShaderPoolImpl::ShaderPoolImpl() {
    registry_.on_construct<WatchedFile>().connect<&start_watching>();
    registry_.on_destroy  <WatchedFile>().connect<&stop_watching> ();
}




namespace {


/*
This is the intermediate state used for loading,
but it also somewhat resembles the structure
as it appears later in the registry.
*/


struct PrimaryDesc {
    File                     file;
    std::unordered_set<File> included;
};


struct ProgramDesc {
    ProgramDefines                                defines;
    std::unordered_map<ShaderTarget, PrimaryDesc> primaries;
};


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
        default: std::terminate();
    }
}


// Expects the program description with the list of primiaries,
// but without includes. Will set the includes of the primaries
// after this call.
//
// Returns a compiled program and the updated list of
// includes for the primaries.
//
// Throws if the compilation/linking failed for any reason.
[[nodiscard]] auto load_and_compile_program(ProgramDesc& inout_program)
    -> UniqueProgram
{
    UniqueProgram program_obj;


    for (auto& [target, primary] : inout_program.primaries) {
        auto source = ShaderSource(read_file(primary.file));

        // Recipe to resolve includes:
        //
        // 1. Find include directive and extract relative path.
        // 2. Get canonical path of the include; fail if no file.
        // 3. Check if it has already been included, if it has:
        //   - Remove the #include directive (behave like #pragma once).
        //   - Repeat from (1).
        // 4. Try to read file of the include; fail if can't read.
        // 5. Replace the #include with the contents of the new file.
        // 6. Add canonical path of the new include to the "included set".
        // 7. Repeat from (1) until no more #includes are found.
        // 8. Create a new FileID to represent each included file,
        //    emplace File into it and attach it to primary.

        const Path& file_path  = primary.file.path();
        const Path  parent_dir = file_path.parent_path();

        primary.included.clear();

        while (std::optional include_dir = ShaderSource::find_include_directive(source)) {

            const Path relative_path = include_dir->path.view();
            File canonical_file = File(std::filesystem::canonical(parent_dir / relative_path)); // Can fail.

            if (primary.included.contains(canonical_file)) {
                // Already included. Just erase the #include line.
                source.remove_subrange(include_dir->full);
            } else {
                ShaderSource included_contents{ read_file(canonical_file) }; // Can fail.
                source.replace_subrange(include_dir->full, included_contents);
                primary.included.emplace(std::move(canonical_file));
            }
        }


        // Now we need to define all the defines that we had prepared.

        for (const std::string& define_str : inout_program.defines.values) {
            // `define_str` is already a full "#define NAME value" directive.
            if (std::optional version_dir = ShaderSource::find_version_directive(source)) {
                source.insert_line_on_line_after(version_dir->full.begin(), define_str);
            } else {
                source.insert_line_on_line_before(source.begin(), define_str);
            }
        }


        // Okay, cool, now we get to actually compile the shader from primary source.

        AnyShader shader = make_any_shader(target);

        auto compile_and_attach = [&](auto& shader_obj) {
            shader_obj->set_source(source.text_view());
            shader_obj->compile();
            if (!shader_obj->has_compiled_successfully()) {
                throw error::ShaderCompilationFailure(
                    file_path.string() + '\n' + shader_obj->get_info_log(),
                    target
                );
            }
            program_obj->attach_shader(shader_obj);
        };

        visit(compile_and_attach, shader);

        // NOTE: `shader` is discarded here.
    }


    program_obj->link();

    if (!program_obj->has_linked_successfully()) {
        throw error::ProgramLinkingFailure(program_obj->get_info_log());
    }

    return program_obj;
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


    // Prepapre the program description.

    ProgramDesc program_desc{ .defines = defines, .primaries = {} };

    if (files.vert) { program_desc.primaries.emplace(ShaderTarget::VertexShader,         PrimaryDesc{ .file = *files.vert, .included = {} }); }
    if (files.tesc) { program_desc.primaries.emplace(ShaderTarget::TessControlShader,    PrimaryDesc{ .file = *files.tesc, .included = {} }); }
    if (files.tese) { program_desc.primaries.emplace(ShaderTarget::TessEvaluationShader, PrimaryDesc{ .file = *files.tese, .included = {} }); }
    if (files.geom) { program_desc.primaries.emplace(ShaderTarget::GeometryShader,       PrimaryDesc{ .file = *files.geom, .included = {} }); }
    if (files.frag) { program_desc.primaries.emplace(ShaderTarget::FragmentShader,       PrimaryDesc{ .file = *files.frag, .included = {} }); }
    if (files.comp) { program_desc.primaries.emplace(ShaderTarget::ComputeShader,        PrimaryDesc{ .file = *files.comp, .included = {} }); }

    UniqueProgram new_program_obj = load_and_compile_program(program_desc);


    // If the load/compilation/linking succeded, unpack the description
    // into the registry, and install the watches.

    auto& registry = registry_;
    const Handle new_program{ registry, registry.create() };

    new_program.emplace<ProgramName>(std::move(program_name));
    new_program.emplace<ProgramDefines>(defines);
    new_program.emplace<UniqueProgram>(std::move(new_program_obj));

    for (const auto& [target, primary_desc] : program_desc.primaries) {
        const Handle new_primary{ registry, registry.create() };
        attach_to_parent(new_primary, new_program.entity());
        new_primary.emplace<ShaderTarget>(target           );
        new_primary.emplace<File>        (primary_desc.file);
        new_primary.emplace<WatchedFile> (&watcher_        );

        for (const File& secondary : primary_desc.included) {
            const Handle new_secondary{ registry, registry.create() };
            attach_to_parent(new_secondary, new_primary.entity());
            new_secondary.emplace<File>       (secondary);
            new_secondary.emplace<WatchedFile>(&watcher_);
        }
    }


    // Cache the program entity for this combination of stages/defines.
    //
    // The associated entity never changes for the given program name,
    // even if the actual UniqueProgram component is replaced later
    // in hot/forced reloading.

    program_map_.insert_or_assign(new_program.get<ProgramName>(), new_program.entity());


    return ShaderToken(new_program.entity(), this);
}


bool ShaderPoolImpl::supports_hot_reload() const noexcept {
    return ShaderWatcher::actually_works;
}



void ShaderPoolImpl::sweep_reload_marked() {
    auto& registry = registry_;

    // Sweep-reload each program independently.
    // We just drop the whole program and reload it again. This is much simpler.
    for (const ProgramID program : registry.view<MarkedForReload>()) {
        const Handle program_handle{ registry, program };

        // First try reloading the whole program, and report if it failed.


        // Prepare the program description from existing structure.

        ProgramDesc program_desc{
            .defines   = has_component<ProgramDefines>(program_handle) ? program_handle.get<ProgramDefines>() : ProgramDefines{},
            .primaries = {}
        };

        program_desc.primaries.reserve(program_handle.get<AsParent>().num_children);

        for (const Handle primary : view_child_handles(program_handle)) {
            program_desc.primaries.emplace(
                primary.get<ShaderTarget>(),
                PrimaryDesc{
                    .file     = primary.get<File>(),
                    .included = {}, // Reset, since we don't know what the new includes are.
                }
            );
        }

        try {
            // Load/compile/link the new program. *This can easily fail*.
            // If it succeeds, we replace the current program object and
            // proceed to resetting the structure in the registry.
            program_handle.get<UniqueProgram>() = load_and_compile_program(program_desc);
        } catch (const std::exception& e) {
            logstream() << "[SHADER RELOAD FAILED]: " << e.what() << '\n';
            // On failure just skip the iteration. Don't touch the registry or anything.
            continue;
        }

        // We don't need to reset everything here.
        //
        // What stays:
        //  - ProgramName, ProgramDefines
        //  - List of Primary Files and their Targets
        //
        // What gets reset:
        //  - UniqueProgram (already done above)
        //  - All secondary (include) files are destroyed
        //  - All watches of secondaries are destroyed too
        //  - Secondaries and their watches are created anew
        //

        for (const Handle primary : view_child_handles(program_handle)) {
            for (const Handle secondary : view_child_handles(primary)) {
                mark_for_destruction(secondary);
            }
            if (has_children(primary)) {
                detach_all_children(primary);
            }
        }

        // NOTE: Will automatically unwatch files on destruction.
        sweep_marked_for_destruction(registry);


        // Now recreate the secondaries again.

        for (const Handle primary : view_child_handles(program_handle)) {
            for (const File& secondary_file : program_desc.primaries.at(primary.get<ShaderTarget>()).included) {
                const Handle new_secondary{ registry, registry.create() };
                attach_to_parent(new_secondary, primary.entity());
                new_secondary.emplace<File>       (secondary_file);
                new_secondary.emplace<WatchedFile>(&watcher_);
            }
        }

    }


    registry_.clear<MarkedForReload>();
}




void ShaderPoolImpl::hot_reload() {

    // Mark roots of each tree for reload.
    while (const std::optional<ShaderWatcher::FileID> modified = watcher_.get_next_modified()) {
        const FileID entity{ *modified };
        const Handle handle{ registry_, entity };

        auto root = get_root_handle(handle);

        // Roots are always Programs.
        assert(has_component<UniqueProgram>(root));

        root.emplace_or_replace<MarkedForReload>();
    };

    // Then sweep.
    sweep_reload_marked();
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
