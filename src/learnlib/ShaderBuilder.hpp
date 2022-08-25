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
    ShaderBuilder& load_frag(const std::string& path) {
        compile_from_source_and_attach(FileReader{}(path), gl::GL_FRAGMENT_SHADER);
        return *this;
    }

    ShaderBuilder& load_vert(const std::string& path) {
        compile_from_source_and_attach(FileReader{}(path), gl::GL_VERTEX_SHADER);
        return *this;
    }

    ShaderBuilder& add_frag(const ShaderSource& source) {
        compile_from_source_and_attach(source, gl::GL_FRAGMENT_SHADER);
        return *this;
    }

    ShaderBuilder& add_vert(const ShaderSource& source) {
        compile_from_source_and_attach(source, gl::GL_VERTEX_SHADER);
        return *this;
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
