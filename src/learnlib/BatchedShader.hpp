#pragma once
#include "GLObjects.hpp"
#include "Logging.hpp"
#include <concepts>
#include <glbinding/gl/boolean.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions-patches.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/types.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>


// This whole idea is pretty much an experimental mess
// Is sorta works and it is sorta faster, sometimes, but still quite messy.
// The worst part is that I can't yet seem to figure out how to make
// it usable in a straightforward fashion.

namespace learn {

namespace leaksgl {

using namespace gl;

struct UniformSetVisitor {
    GLint index;
    UniformSetVisitor(GLint index) : index( index ) {}

    void operator()(glm::float32_t v) const { glUniform1f(index, v); }
    void operator()(glm::vec2 v) const { glUniform2fv(index, 1, glm::value_ptr(v)); }
    void operator()(glm::vec3 v) const { glUniform3fv(index, 1, glm::value_ptr(v)); }
    void operator()(glm::vec4 v) const { glUniform4fv(index, 1, glm::value_ptr(v)); }

    void operator()(glm::int32_t v) const { glUniform1i(index, v); }
    void operator()(glm::ivec2 v) const { glUniform2iv(index, 1, glm::value_ptr(v)); }
    void operator()(glm::ivec3 v) const { glUniform3iv(index, 1, glm::value_ptr(v)); }
    void operator()(glm::ivec4 v) const { glUniform4iv(index, 1, glm::value_ptr(v)); }

    void operator()(glm::uint32_t v) const { glUniform1ui(index, v); }
    void operator()(glm::uvec2 v) const { glUniform2uiv(index, 1, glm::value_ptr(v)); }
    void operator()(glm::uvec3 v) const { glUniform3uiv(index, 1, glm::value_ptr(v)); }
    void operator()(glm::uvec4 v) const { glUniform4uiv(index, 1, glm::value_ptr(v)); }

    void operator()(bool v) const { glUniform1ui(index, v); }
    void operator()(glm::bvec2 v) const { glUniform2ui(index, v.x, v.y); }
    void operator()(glm::bvec3 v) const { glUniform3ui(index, v.x, v.y, v.z); }
    void operator()(glm::bvec4 v) const { glUniform4ui(index, v.x, v.y, v.z, v.w); }

    void operator()(glm::mat2 v) const { glUniformMatrix2fv(index, 1, GL_FALSE, glm::value_ptr(v)); }
    void operator()(glm::mat2x3 v) const { glUniformMatrix2x3fv(index, 1, GL_FALSE, glm::value_ptr(v)); }
    void operator()(glm::mat2x4 v) const { glUniformMatrix2x4fv(index, 1, GL_FALSE, glm::value_ptr(v)); }

    void operator()(glm::mat3 v) const { glUniformMatrix3fv(index, 1, GL_FALSE, glm::value_ptr(v)); }
    void operator()(glm::mat3x2 v) const { glUniformMatrix3x2fv(index, 1, GL_FALSE, glm::value_ptr(v)); }
    void operator()(glm::mat3x4 v) const { glUniformMatrix3x4fv(index, 1, GL_FALSE, glm::value_ptr(v)); }

    void operator()(glm::mat4 v) const { glUniformMatrix4fv(index, 1, GL_FALSE, glm::value_ptr(v)); }
    void operator()(glm::mat4x2 v) const { glUniformMatrix4x2fv(index, 1, GL_FALSE, glm::value_ptr(v)); }
    void operator()(glm::mat4x3 v) const { glUniformMatrix4x3fv(index, 1, GL_FALSE, glm::value_ptr(v)); }
};

} // namespace leaksgl

// There's a strong assumption here that there exists a 1-to-1
// correspondence between GLSL types and C++ types, with exception
// of samplers, since they are basically integers.
//
// What this means, however, is that there's only one representation
// for each type, so vec2 is only glm::vec2 and not float[2].
// This simplifies the mess of type checking significantly and
// unbloats the variants.

using leaksgl::UniformSetVisitor;

// Oh, yeah, the scalar types are also in the vector variant because.
using VecUniformVariant = std::variant<
    glm::float32_t, glm::vec2, glm::vec3, glm::vec4,
    glm::int32_t, glm::ivec2, glm::ivec3, glm::ivec4,
    glm::uint32_t, glm::uvec2, glm::uvec3, glm::uvec4,
    bool, glm::bvec2, glm::bvec3, glm::bvec4
>;

using MatUniformVariant = std::variant<
    glm::mat2, glm::mat2x3, glm::mat2x4,
    glm::mat3, glm::mat3x2, glm::mat3x4,
    glm::mat4, glm::mat4x2, glm::mat4x3
>;


struct UniformInfo {
    gl::GLint index;
    gl::GLenum type;
};

namespace leaksgl {

using namespace gl;

// Ughhh, type traits because I want to compare integers to types...

template<typename T>
struct SymConstant;

#define MAKE_SYM(type, val)                   \
    template<>                                \
    struct SymConstant<type> {                \
        static constexpr GLenum value{ val }; \
    };

MAKE_SYM(glm::float32_t, GL_FLOAT);
MAKE_SYM(glm::vec2, GL_FLOAT_VEC2);
MAKE_SYM(glm::vec3, GL_FLOAT_VEC3);
MAKE_SYM(glm::vec4, GL_FLOAT_VEC4);
MAKE_SYM(glm::int32_t, GL_INT);
MAKE_SYM(glm::ivec2, GL_INT_VEC2);
MAKE_SYM(glm::ivec3, GL_INT_VEC3);
MAKE_SYM(glm::ivec4, GL_INT_VEC4);
MAKE_SYM(glm::uint32_t, GL_UNSIGNED_INT);
MAKE_SYM(glm::uvec2, GL_UNSIGNED_INT_VEC2);
MAKE_SYM(glm::uvec3, GL_UNSIGNED_INT_VEC3);
MAKE_SYM(glm::uvec4, GL_UNSIGNED_INT_VEC4);
MAKE_SYM(bool, GL_BOOL);
MAKE_SYM(glm::bvec2, GL_BOOL_VEC2);
MAKE_SYM(glm::bvec3, GL_BOOL_VEC3);
MAKE_SYM(glm::bvec4, GL_BOOL_VEC4);

MAKE_SYM(glm::mat2, GL_FLOAT_MAT2);
MAKE_SYM(glm::mat2x3, GL_FLOAT_MAT2x3);
MAKE_SYM(glm::mat2x4, GL_FLOAT_MAT2x4);
MAKE_SYM(glm::mat3, GL_FLOAT_MAT3);
MAKE_SYM(glm::mat3x2, GL_FLOAT_MAT3x2);
MAKE_SYM(glm::mat3x4, GL_FLOAT_MAT3x4);
MAKE_SYM(glm::mat4, GL_FLOAT_MAT4);
MAKE_SYM(glm::mat4x2, GL_FLOAT_MAT4x2);
MAKE_SYM(glm::mat4x3, GL_FLOAT_MAT4x3);

#undef MAKE_SYM

} // namespace leaksgl

using leaksgl::SymConstant;



template<typename T, typename ...Us>
concept is_any_of = (std::same_as<T, Us> || ...);

template<typename T>
concept is_scalar_or_vector_type = is_any_of<
    T,
    glm::float32_t,   glm::vec2,  glm::vec3,  glm::vec4,
    glm::int32_t,     glm::ivec2, glm::ivec3, glm::ivec4,
    glm::uint32_t,    glm::uvec2, glm::uvec3, glm::uvec4,
    bool,             glm::bvec2, glm::bvec3, glm::bvec4
>;

template<typename T>
concept is_matrix_type = is_any_of<
    T,
    glm::mat2, glm::mat2x3, glm::mat2x4,
    glm::mat3, glm::mat3x2, glm::mat3x4,
    glm::mat4, glm::mat4x2, glm::mat4x3
>;



class UniformMap {
private:
    // This subgrouping is an implementation headache,
    // but supposedly is a going to help with the
    // variant bloat. Premature optimization in a way,
    // but why would float take the space of mat4?
    //
    // Maybe don't do this actually. What kind of cache locality
    // performance we're gaining when at worst there's like
    // ~20 uniforms in a shader.
    std::unordered_map<gl::GLint, VecUniformVariant> vectors_;
    std::unordered_map<gl::GLint, MatUniformVariant> matrices_;

public:
    void apply_all() const {
        for (auto& elem : vectors_) {
            std::visit(UniformSetVisitor(elem.first), elem.second);
        }
        for (auto& elem : matrices_) {
            std::visit(UniformSetVisitor(elem.first), elem.second);
        }
    }

    template<typename Arg>
    requires
        (is_scalar_or_vector_type<std::remove_cvref_t<Arg>> && !is_any_of<std::remove_cvref_t<Arg>, glm::int32_t, glm::uint32_t>)
        || is_matrix_type<std::remove_cvref_t<Arg>>
    void set(UniformInfo info, Arg&& arg) {
        // All of those type traits for the sake of this assert.
        assert(info.type == SymConstant<std::remove_cvref_t<Arg>>::value);

        if constexpr (is_scalar_or_vector_type<std::remove_cvref_t<Arg>>) {
            vectors_.insert_or_assign(info.index, arg);
        } else {
            matrices_.insert_or_assign(info.index, arg);
        }
    }

    // Sampler types can be either int or unsigned int.
    // So we check on insertion but don't store them as
    // separate types.
    template<std::same_as<glm::int32_t> T>
    void set(UniformInfo info, T&& arg) {
        using namespace gl;
        auto type = info.type;
        assert(type == GL_INT || is_sampler_type(type));
        vectors_.insert_or_assign(info.index, arg);
    }

    template<std::same_as<glm::uint32_t> T>
    void set(UniformInfo info, T&& arg) {
        using namespace gl;
        auto type = info.type;
        assert(type == GL_UNSIGNED_INT || is_sampler_type(type));
        vectors_.insert_or_assign(info.index, arg);
    }

    static bool is_sampler_type(gl::GLenum type) noexcept {
        using namespace gl;
        auto eq_any_of = [](gl::GLenum type, auto... args) {
            return ((type == args) || ...);
        };
        return eq_any_of(type,
            GL_SAMPLER_1D, GL_SAMPLER_2D, GL_SAMPLER_3D,
            GL_SAMPLER_CUBE,
            GL_SAMPLER_1D_SHADOW, GL_SAMPLER_2D_SHADOW,
            GL_SAMPLER_1D_ARRAY, GL_SAMPLER_2D_ARRAY,
            GL_SAMPLER_1D_ARRAY_SHADOW, GL_SAMPLER_2D_ARRAY_SHADOW,
            GL_SAMPLER_2D_MULTISAMPLE, GL_SAMPLER_2D_MULTISAMPLE_ARRAY,
            GL_SAMPLER_CUBE_SHADOW,
            GL_SAMPLER_BUFFER,
            GL_SAMPLER_2D_RECT, GL_SAMPLER_2D_RECT_SHADOW,
            GL_INT_SAMPLER_1D, GL_INT_SAMPLER_2D, GL_INT_SAMPLER_3D,
            GL_INT_SAMPLER_CUBE,
            GL_INT_SAMPLER_1D_ARRAY, GL_INT_SAMPLER_2D_ARRAY,
            GL_INT_SAMPLER_2D_MULTISAMPLE, GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
            GL_INT_SAMPLER_BUFFER,
            GL_INT_SAMPLER_2D_RECT,
            GL_UNSIGNED_INT_SAMPLER_1D, GL_UNSIGNED_INT_SAMPLER_2D, GL_UNSIGNED_INT_SAMPLER_3D,
            GL_UNSIGNED_INT_SAMPLER_CUBE,
            GL_UNSIGNED_INT_SAMPLER_1D_ARRAY, GL_UNSIGNED_INT_SAMPLER_2D_ARRAY,
            GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE, GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
            GL_UNSIGNED_INT_SAMPLER_BUFFER,
            GL_UNSIGNED_INT_SAMPLER_2D_RECT
        );
    }
};




class UniformInfoMap {
private:
    std::unordered_map<std::string, UniformInfo> info_;

public:
    void emplace(const std::string& name, UniformInfo info) {
        info_.emplace(name, info);
    }

    void emplace(std::string_view name, UniformInfo info) {
        info_.emplace(std::string(name), info);
    }

    gl::GLint location_of(const std::string& name) const {
        return info_.at(name).index;
    }

    UniformInfo info_of(const std::string& name) const {
        return info_.at(name);
    }

    const auto& map() const noexcept {
        return info_;
    }

};

// A simple? class that composes together a uniform map
// and an associated ShaderProgram. Allows you to
// set uniforms preemptivly and lazily 'apply' them at a later stage.
//
// Most useful for scenarios where there's only a single
// draw call per shader, such as postprocessing.
//
// Seemingly unneccessary at first, but maybe is an okay solution
// when you want to configure the postprocessing chain dynamically:
// add/remove/reorder shaders, change the uniforms, etc.
class BatchedShader {
private:
    ShaderProgram shader_;
    UniformInfoMap uniform_info_;
    UniformMap uniforms_;

    void query_uniforms() {
        using namespace gl;

        GLint num_uniforms;
        glGetProgramiv(shader_, GL_ACTIVE_UNIFORMS, &num_uniforms);

        GLint max_name_size;
        glGetProgramiv(shader_, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_size);

        std::vector<GLchar> buffer(static_cast<size_t>(max_name_size));

        for (GLint i{ 0 }; i < num_uniforms; ++i) {
            GLint uniform_size;
            GLenum uniform_type;
            GLsizei name_size;
            glGetActiveUniform(shader_, i, max_name_size, &name_size, &uniform_size, &uniform_type, buffer.data());
            uniform_info_.emplace(
                std::string_view(buffer.begin(), buffer.begin() + name_size),
                { i, uniform_type }
            );
        }


    }

public:
    BatchedShader(ShaderProgram&& shader)
        : shader_{ std::move(shader) }
    {
        query_uniforms();
    }

    ShaderProgram& program() noexcept { return shader_; }

    gl::GLint location_of(const std::string& name) const {
        return uniform_info_.location_of(name);
    }

    UniformMap& map() noexcept { return uniforms_; }

    // This is mostly for testing.
    // Make sure the current program is active.
    void uniform(const std::string& name, auto... args) const {
        ActiveShaderProgram::uniform(uniform_info_.location_of(name), args...);
	}

    // More testing
    void print_uniforms() const {
        for (auto& info : uniform_info_.map()) {
            global_logstream
                << "[Uniform] name: " << info.first
                << ", index: " << info.second.index
                << ", type: " << static_cast<std::underlying_type_t<decltype(info.second.type)>>(info.second.type)
                << "\n";
        }
    }

    gl::GLenum type_of(const std::string& name) const {
        return uniform_info_.info_of(name).type;
    }

    UniformInfo info_of(const std::string& name) const {
        return uniform_info_.info_of(name);
    }

    void set(/* string name, variant_t value */) {
        // TODO: Do something?
    }

    // Applies the uniforms, that have been set in the map before.
    // ONLY the ones that have been set.
    // Want them to not be applied? Don't set them.
    //
    // Yeah, that's what I meant by 'not straightforward'
    ActiveShaderProgram apply() {
        auto asp = shader_.use();
        map().apply_all();
        return asp;
    }


};



} // namespace learn
