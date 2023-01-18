#pragma once
#include "GLObjects.hpp"
#include "Logging.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions-patches.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/types.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace learn {


struct UniformInfo {
    gl::GLint index;
    gl::GLint size;
    gl::GLenum type;
};


class UniformMap {
private:
    std::unordered_map<std::string, UniformInfo> info_;

public:
    void emplace(const std::string& name, UniformInfo info) {
        info_.emplace(name, info);
    }

    void emplace(std::string_view name, UniformInfo info) {
        info_.emplace(std::string(name), info);
    }

    gl::GLint location_of(const std::string& name) {
        return info_.at(name).index;
    }

    const auto& map() const noexcept {
        return info_;
    }

    // TODO: implement later
    //
    // A wrapper around a map:
    //
    //     uniform_name -> pair<location, value>
    //
    // where 'value' is a variant of glsl types
    //
    //     using value =
    //         variant<float, array<float, 2>, ..., glm::vec2, glm::vec3, ..., glm::mat4, ...>;
    //
};

// A simple class that composes together a uniform map
// and an associated ShaderProgram. Allows you to
// set uniforms preemptivly and lazily 'apply' them at a later stage.
//
// Most useful for scenarios where there's only a single
// draw call per shader, such as postprocessing.
class BatchedShader {
private:
    ShaderProgram shader_;
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
            uniforms_.emplace(
                std::string_view(buffer.begin(), buffer.begin() + name_size),
                { i, uniform_size, uniform_type }
            );
        }


    }

public:
    BatchedShader(ShaderProgram&& shader)
        : shader_{ std::move(shader) }
    {
        // TODO:
        // Query the names and types of the uniforms
        // and fill out the uniform map.
        // Use glGetActiveUniform() for querying.
        query_uniforms();
    }

    ShaderProgram& program() noexcept { return shader_; }

    gl::GLint location_of(const std::string& name) {
        return uniforms_.location_of(name);
    }

    // This is mostly for testing.
    // Make sure the current program is active.
    void uniform(const std::string& name, auto... args) {
        ActiveShaderProgram::uniform(uniforms_.location_of(name), args...);
	}

    void print_uniforms() const {
        for (auto& info : uniforms_.map()) {
            global_logstream
                << "[Uniform] name: " << info.first
                << ", index: " << info.second.index
                << ", type: " << static_cast<std::underlying_type_t<decltype(info.second.type)>>(info.second.type)
                << ", size: " << info.second.size
                << "\n";
        }
    }

    gl::GLenum type_of(/* string name */) {
        // TODO:
        // Return the type of the uniform.
        // Make a visitor for a variant.
        return {};
    }

    void set(/* string name, variant_t value */) {
        // TODO:
        // Set the value of uniform with 'name',
        // but check that the type is the same.
        //
        //     if (value.index() == uniforms_[name].value.index()) {
        //          uniforms_[name].value = value;
        //     } else {
        //         report_uniform_failure();
        //     }
        //
    }

    ActiveShaderProgram apply() {
        // TODO:
        // Make the shader active and set the uniforms
        auto asp = shader_.use();
        // for (auto&& elem : uniforms_) {
        //     std::visit(
        //         UniformApplierVisitor(asp, elem.second.location),
        //         elem.second.value
        //     );
        // }
        return asp;
    }


};



} // namespace learn
