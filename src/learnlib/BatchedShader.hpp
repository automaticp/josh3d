#pragma once
#include "GLObjects.hpp"
#include <glbinding/gl/gl.h>

namespace learn {

class UniformMap {
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

public:
    BatchedShader(/* shaders */)
    // : shader_{
    //     ShaderBuilder().add_...(shaders).get()
    // }
    {
        // TODO:
        // Query the names and types of the uniforms
        // and fill out the uniform map.
        // Use glGetActiveUniform() for querying.
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
