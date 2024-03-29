#pragma once
#include "Filesystem.hpp"
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "GLShaders.hpp"
#include "ReadFile.hpp"
#include "ShaderSource.hpp"
#include <string>
#include <utility>
#include <vector>


namespace josh {


class ShaderBuilder {
private:
    struct UnevaluatedShader {
        ShaderSource source;
        GLenum type;
    };

    struct ShaderDefine {
        std::string name;
        std::string value;

        std::string get_define_string() const {
            return "#define " + name + " " + value;
        }
    };

    std::vector<UnevaluatedShader> shaders_;
    std::vector<ShaderDefine> defines_;

public:
    ShaderBuilder& load_shader(const File& file, GLenum type) {
        shaders_.emplace_back(ShaderSource(read_file(file)), type);
        return *this;
    }

    ShaderBuilder& load_frag(const File& file) {
        return load_shader(file, gl::GL_FRAGMENT_SHADER);
    }

    ShaderBuilder& load_vert(const File& file) {
        return load_shader(file, gl::GL_VERTEX_SHADER);
    }

    ShaderBuilder& load_geom(const File& file) {
        return load_shader(file, gl::GL_GEOMETRY_SHADER);
    }

    ShaderBuilder& load_comp(const File& file) {
        return load_shader(file, gl::GL_COMPUTE_SHADER);
    }



    ShaderBuilder& add_shader(const ShaderSource& source, GLenum type) {
        shaders_.emplace_back(source, type);
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



    ShaderBuilder& define(std::string name) {
        return define(std::move(name), "1");
    }

    ShaderBuilder& define(std::string name, std::string value) {
        defines_.emplace_back(std::move(name), std::move(value));
        return *this;
    }


    [[nodiscard]] UniqueShaderProgram get();

};




[[nodiscard]]
inline auto ShaderBuilder::get()
    -> UniqueShaderProgram
{
    UniqueShaderProgram sp;

    auto compile_from_source_and_attach =
        [&sp](const UnevaluatedShader& shader_info) {
            UniqueShader new_shader{ shader_info.type };
            new_shader.set_source(shader_info.source.text().c_str());
            new_shader.compile();
            sp.attach_shader(new_shader);
        };

    for (auto& shader : shaders_) {

        for (auto& define : defines_) {
            const bool was_found =
                shader.source.find_and_insert_as_next_line("#version", define.get_define_string());

            assert(was_found);
        }
        compile_from_source_and_attach(shader);

    }

    sp.link();

    return sp;
}




} // namespace josh
