#pragma once
#include "Common.hpp"
#include "Filesystem.hpp"
#include "GLMutability.hpp"
#include "GLProgram.hpp"
#include <entt/entity/fwd.hpp>
#include <memory>


/*
TODO: Shaders *really* want to be a resource, but in the current
implementation they are their own thing entirely. Hey, at least
they support hot reloading.
*/
namespace josh {


class ShaderPool;
class ShaderPoolImpl;


/*
Returns a thread_local `ShaderPool`.

Must be accessed after creating `GlobalContext` in the same thread,
or by calling `init_thread_local_shader_pool()` and subsequently
`clear_thread_local_shader_pool()` before destroying OpenGL context.
*/
auto shader_pool() -> ShaderPool&;
void init_thread_local_shader_pool();
void clear_thread_local_shader_pool();

/*
TODO: We should probably have a `shader_pool().get(token)`
interface instead of the current `token.get()`.
The latter is "more convenient" but it hides the "immediate"
nature of tokens and deviates from how we treat other ID handles.
*/
class ShaderToken
{
public:
    auto get()       noexcept -> RawProgram<GLMutable>;
    auto get() const noexcept -> RawProgram<GLConst>;

    operator RawProgram<GLMutable> ()       noexcept { return get(); }
    operator RawProgram<GLConst>   () const noexcept { return get(); }

private:
    using ProgramID = entt::entity;
    ShaderToken(ProgramID id, ShaderPoolImpl* pool) : id_{ id }, pool_{ pool } {}

    ProgramID       id_;
    ShaderPoolImpl* pool_;

    friend ShaderPool;
    friend ShaderPoolImpl;
};


struct ProgramDefines
{
    HashSet<String> values;

    auto define(const char* name, auto&& value) -> ProgramDefines&;
};

auto ProgramDefines::define(const char* name, auto&& value)
    -> ProgramDefines&
{
    values.emplace(fmt::format("#define {} {}", name, value));
    return *this;
}

/*
TODO: Remove `File`. Use `Path`.

HMM: This could support a nicer way of specifying defines
right in the struct itself, so that:
    shader_pool().create({
        .vert = "path/to/shader.vert",
        .frag = "path/to/shader.frag",
        .defines = {
            { "ENABLE_BLAH",        1 },
            { "CAMERA_UBO_BINDING", 2 },
        },
    });
is possible in some way. The `defines` would likely have to be
some nonsense, either an Tuple<Pair<String, ...T>> with CTAD
(not sure if it works with aggregates), or the RHS of each
entry would be a Variant of sorts.

HMM: Shader defines are pretty much "import metadata"...
*/
struct ProgramFiles
{
    Optional<File> vert{};
    Optional<File> geom{};
    Optional<File> tesc{};
    Optional<File> tese{};
    Optional<File> frag{};
    Optional<File> comp{};
};

/*
TODO: Below works actually, so maybe just integrate it?
*/
#if 0
struct ProgramSpec
{
    Optional<Path> vert = nullopt;
    Optional<Path> geom = nullopt;
    Optional<Path> tesc = nullopt;
    Optional<Path> tese = nullopt;
    Optional<Path> frag = nullopt;
    Optional<Path> comp = nullopt;
    using DefineValue = Variant<i32, i64, u32, u64, String>;
    HashMap<String, DefineValue>
                   defines = {};
};

inline const auto example = ProgramSpec{
    .vert = "path/to/shader.vert",
    .frag = "path/to/shader.frag",
    .defines = {
        { "ENABLE_BLAH",        1 },
        { "CAMERA_UBO_BINDING", 2 },
    },
};
#endif


/*
FIXME: It's primarily the file watcher that needs pimpl.
In hindsight, the watcher has nothing to do with the pool itself.
And the "hot reloading" of the pool is simply about being able to do
    `shader_pool().reload(shader_token)`
And when and which shader is decided by the watcher or another
hot-reloader system. So the current organization is pretty useless.
*/
class ShaderPool
{
public:
    // Get or create a shader program associated with the specified
    // set of `program_files`, and return a `ShaderToken` connected to it.
    [[nodiscard]] auto get(const ProgramFiles& program_files) -> ShaderToken;

    // Get or create a shader program associated with the specified
    // set of `program_files` and `defines`, and return a `ShaderToken` connected to it.
    [[nodiscard]] auto get(const ProgramFiles& program_files, const ProgramDefines& defines) -> ShaderToken;

    // Whether hot reloading is supported.
    auto supports_hot_reload() const noexcept -> bool;

    // Lazily reload and recompile modified shaders and it's users only.
    // Will throw if hot reloading is not supported.
    void hot_reload();

    // Forcefully reload and recompile all shaders connected to the pool.
    // Alternative for when hot-reloading is not available.
    //
    // WARNING: Very slow, don't call every frame.
    void force_reload();

    ShaderPool();
    ~ShaderPool();

private:
    std::unique_ptr<ShaderPoolImpl> pimpl_;
};


} // namespace josh
