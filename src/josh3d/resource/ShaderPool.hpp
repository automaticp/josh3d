#pragma once
#include "Filesystem.hpp"
#include "GLMutability.hpp"
#include "GLProgram.hpp"
#include <entt/entity/fwd.hpp>
#include <memory>
#include <optional>
#include <unordered_set>


namespace josh {


class ShaderPool;
class ShaderPoolImpl;


// Returns a thread_local `ShaderPool`.
//
// Must be accessed after creating `GlobalContext` in the same thread,
// or by calling `init_thread_local_shader_pool()` and subsequently
// `clear_thread_local_shader_pool()` before destroying OpenGL context.
auto shader_pool() -> ShaderPool&;
void init_thread_local_shader_pool();
void clear_thread_local_shader_pool();




class ShaderToken {
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




struct ProgramDefines {
    std::unordered_set<std::string> values;

    auto define(const char* name, auto&& value) -> ProgramDefines&;
};


inline auto ProgramDefines::define(const char* name, auto&& value)
    -> ProgramDefines&
{
    std::stringstream ss;
    ss << "#define " << name << ' ' << value;
    values.emplace(std::move(ss).str());
    return *this;
}


struct ProgramFiles {
    std::optional<File> vert{};
    std::optional<File> geom{};
    std::optional<File> tesc{};
    std::optional<File> tese{};
    std::optional<File> frag{};
    std::optional<File> comp{};
};




class ShaderPool {
public:

    // Get or create a shader program associated with the specified
    // set of `program_files`, and return a `ShaderToken` connected to it.
    [[nodiscard]] auto get(const ProgramFiles& program_files) -> ShaderToken;

    // Get or create a shader program associated with the specified
    // set of `program_files` and `defines`, and return a `ShaderToken` connected to it.
    [[nodiscard]] auto get(const ProgramFiles& program_files, const ProgramDefines& defines) -> ShaderToken;

    // Whether hot reloading is supported.
    bool supports_hot_reload() const noexcept;

    // Lazily reload and recompile modified shaders and it's users only.
    // Will throw if hot reloading is not supported.
    void hot_reload();

    // Forcefully reload and recompile all shaders connected to the pool.
    // Alternative for when hot-reloading is not available.
    //
    // WARN: Very slow, don't call every frame.
    void force_reload();

    ShaderPool();
    ~ShaderPool();

private:
    std::unique_ptr<ShaderPoolImpl> pimpl_;
};




} // namespace josh
