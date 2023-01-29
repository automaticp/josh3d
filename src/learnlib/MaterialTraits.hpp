#pragma once
#include "GLObjects.hpp"

#include <glbinding/gl/gl.h>
#include <array>


namespace learn {

enum class TextureType {
    diffuse, specular
};


struct MaterialParams {
    const gl::GLchar* name;
    TextureType tex_type;
    gl::GLenum target;
    gl::GLenum tex_unit;

    gl::GLint sampler_uniform() const noexcept {
        using gl::GLint;
        return static_cast<GLint>(tex_unit) - static_cast<GLint>(gl::GL_TEXTURE0);
    }
};


// These traits are like implementing a static interface except
// in a very awkward upside-down way and not related to a single class
// in particular.
//
// Perhaps one day I'll figure out a better way.
template<typename M>
struct MaterialTraits {
    static_assert(sizeof(M) == 0, "Custom material layout M must have a MaterialTraits<M> specialization.");

    constexpr static std::array<MaterialParams, 0> texparams{};

    using locations_type = std::array<gl::GLint, 0>;
};


// Provide specializations for your own material layouts.

template<typename MaterialT>
void apply_material(ActiveShaderProgram& asp, const MaterialT& mat,
    const typename MaterialTraits<MaterialT>::locations_type& uniform_locations);

template<typename MaterialT>
void apply_material(ActiveShaderProgram& asp, const MaterialT& mat);


template<typename M>
typename MaterialTraits<M>::locations_type query_locations(ShaderProgram& sp);



} // namespace learn
