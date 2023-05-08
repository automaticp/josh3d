#pragma once
#include <utility>
#include <string>

#include "GLObjects.hpp"
#include "ShaderSource.hpp"


namespace learn {



class ShaderBuilder {
private:
    ShaderProgram sp_{};

public:
    ShaderBuilder& load_shader(const std::string& path, gl::GLenum type) {
        compile_from_source_and_attach(ShaderSource::from_file(path), type);
        return *this;
    }

    ShaderBuilder& load_frag(const std::string& path) {
        return load_shader(path, gl::GL_FRAGMENT_SHADER);
    }

    ShaderBuilder& load_vert(const std::string& path) {
        return load_shader(path, gl::GL_VERTEX_SHADER);
    }

    ShaderBuilder& load_geom(const std::string& path) {
        return load_shader(path, gl::GL_GEOMETRY_SHADER);
    }

    ShaderBuilder& load_comp(const std::string& path) {
        return load_shader(path, gl::GL_COMPUTE_SHADER);
    }



    ShaderBuilder& add_shader(const ShaderSource& source, gl::GLenum type) {
        compile_from_source_and_attach(source, type);
        return *this;
    }

    ShaderBuilder& add_frag(const ShaderSource& source) {
        return add_shader(source, gl::GL_FRAGMENT_SHADER);
    }

    ShaderBuilder& add_vert(const ShaderSource& source) {
        return add_shader(source, gl::GL_VERTEX_SHADER);
    }

    ShaderBuilder& add_geom(const ShaderSource& source) {
        return add_shader(source, gl::GL_GEOMETRY_SHADER);
    }

    ShaderBuilder& add_comp(const ShaderSource& source) {
        return add_shader(source, gl::GL_COMPUTE_SHADER);
    }



    [[nodiscard]]
    ShaderProgram get() {
        sp_.link();
        return std::move(sp_);
    }

private:
    void compile_from_source_and_attach(const std::string& source, gl::GLenum type) {
        Shader s{ type };
        s.set_source(source).compile();
        sp_.attach_shader(s);
    }

};


} // namespace learn
