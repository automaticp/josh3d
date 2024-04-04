#pragma once
#include "Filesystem.hpp"
#include "GLObjects.hpp"
#include "GLShaders.hpp"
#include "GLProgram.hpp"
#include "ReadFile.hpp"
#include "ShaderSource.hpp"
#include <string>
#include <utility>
#include <vector>


namespace josh {


namespace error {


class ShaderCompilationFailure final : public RuntimeError {
public:
    static constexpr auto prefix = "Failed to Compile Shader: ";

    std::string  info_log;
    ShaderTarget shader_type;

    ShaderCompilationFailure(
        std::string  info_log,
        ShaderTarget shader_type)
        : RuntimeError(prefix, info_log)
        , info_log   { std::move(info_log) }
        , shader_type{ shader_type         }
    {}
};


class ProgramLinkingFailure final : public RuntimeError {
public:
    static constexpr auto prefix = "Failed to Link Program: ";

    std::string info_log;

    ProgramLinkingFailure(std::string info_log)
        : RuntimeError(prefix, info_log)
        , info_log   { std::move(info_log) }
    {}
};


} // namespace error





class ShaderBuilder {
private:
    struct UnevaluatedShader {
        ShaderSource source;
        ShaderTarget type;
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
    ShaderBuilder& load_shader(const File& file, ShaderTarget type) {
        shaders_.emplace_back(ShaderSource(read_file(file)), type);
        return *this;
    }

    ShaderBuilder& load_frag(const File& file) { return load_shader(file, ShaderTarget::FragmentShader);       }
    ShaderBuilder& load_vert(const File& file) { return load_shader(file, ShaderTarget::VertexShader);         }
    ShaderBuilder& load_geom(const File& file) { return load_shader(file, ShaderTarget::GeometryShader);       }
    ShaderBuilder& load_comp(const File& file) { return load_shader(file, ShaderTarget::ComputeShader);        }
    ShaderBuilder& load_tesc(const File& file) { return load_shader(file, ShaderTarget::TessControlShader);    }
    ShaderBuilder& load_tese(const File& file) { return load_shader(file, ShaderTarget::TessEvaluationShader); }

    ShaderBuilder& add_shader(const ShaderSource& source, ShaderTarget type) {
        shaders_.emplace_back(source, type);
        return *this;
    }

    ShaderBuilder& add_frag(const ShaderSource& source) { return add_shader(source, ShaderTarget::FragmentShader);       }
    ShaderBuilder& add_vert(const ShaderSource& source) { return add_shader(source, ShaderTarget::VertexShader);         }
    ShaderBuilder& add_geom(const ShaderSource& source) { return add_shader(source, ShaderTarget::GeometryShader);       }
    ShaderBuilder& add_comp(const ShaderSource& source) { return add_shader(source, ShaderTarget::ComputeShader);        }
    ShaderBuilder& add_tesc(const ShaderSource& source) { return add_shader(source, ShaderTarget::TessControlShader);    }
    ShaderBuilder& add_tese(const ShaderSource& source) { return add_shader(source, ShaderTarget::TessEvaluationShader); }



    ShaderBuilder& define(std::string name) {
        return define(std::move(name), "1");
    }

    ShaderBuilder& define(std::string name, std::string value) {
        defines_.emplace_back(std::move(name), std::move(value));
        return *this;
    }


    [[nodiscard]] UniqueProgram get();

};




[[nodiscard]]
inline auto ShaderBuilder::get()
    -> UniqueProgram
{
    UniqueProgram sp;


    auto compile_and_attach =
        [&sp]<typename UniqueShaderType>(
            UniqueShaderType    new_shader,
            const ShaderSource& source)
        {
            new_shader->set_source(source.text());
            new_shader->compile();
            if (!new_shader->has_compiled_successfully()) {
                throw error::ShaderCompilationFailure(new_shader->get_info_log(), UniqueShaderType::target_type);
            }
            sp->attach_shader(new_shader);
        };


    for (auto& shader : shaders_) {

        for (auto& define : defines_) {
            const bool was_found =
                shader.source.find_and_insert_as_next_line("#version", define.get_define_string());
            assert(was_found);
        }

        switch (shader.type) {
            using enum ShaderTarget;
            case VertexShader:         compile_and_attach(UniqueVertexShader(),         shader.source); break;
            case FragmentShader:       compile_and_attach(UniqueFragmentShader(),       shader.source); break;
            case GeometryShader:       compile_and_attach(UniqueGeometryShader(),       shader.source); break;
            case ComputeShader:        compile_and_attach(UniqueComputeShader(),        shader.source); break;
            case TessControlShader:    compile_and_attach(UniqueTessControlShader(),    shader.source); break;
            case TessEvaluationShader: compile_and_attach(UniqueTessEvaluationShader(), shader.source); break;
            default:
                assert(false);
        }

    }


    sp->link();

    if (!sp->has_linked_successfully()) {
        throw error::ProgramLinkingFailure(sp->get_info_log());
    }

    return sp;
}




} // namespace josh
