#pragma once
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "Filesystem.hpp"
#include "GLObjects.hpp"
#include "ReadFile.hpp"
#include "RuntimeError.hpp"
#include "ShaderSource.hpp"
#include <string>
#include <utility>


namespace josh {
namespace error {

/*
FIXME: I hate the current error conventions.
*/
class ShaderCompilationFailure final : public RuntimeError
{
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

class IncludeResolutionFailure final : public RuntimeError
{
public:
    static constexpr auto prefix = "Failed to Resolve Include: ";

    std::string include_name;

    IncludeResolutionFailure(std::string include_name)
        : RuntimeError(prefix, include_name)
        , include_name{ std::move(include_name) }
    {}
};

class ProgramLinkingFailure final : public RuntimeError
{
public:
    static constexpr auto prefix = "Failed to Link Program: ";

    std::string info_log;

    ProgramLinkingFailure(std::string info_log)
        : RuntimeError(prefix, info_log)
        , info_log   { std::move(info_log) }
    {}
};

} // namespace error
using error::ShaderCompilationFailure;
using error::IncludeResolutionFailure;
using error::ProgramLinkingFailure;


/*
NOTE: This is not used much anymore and is kept more as a reference, I think.
*/
class ShaderBuilder
{
public:
    ShaderBuilder& load_shader(const File& file, ShaderTarget type)
    {
        shaders_.emplace_back(ShaderSource(read_file(file)), type, file.path());
        shaders_.back().resolve_includes();
        return *this;
    }

    ShaderBuilder& load_frag(const File& file) { return load_shader(file, ShaderTarget::Fragment);       }
    ShaderBuilder& load_vert(const File& file) { return load_shader(file, ShaderTarget::Vertex);         }
    ShaderBuilder& load_geom(const File& file) { return load_shader(file, ShaderTarget::Geometry);       }
    ShaderBuilder& load_comp(const File& file) { return load_shader(file, ShaderTarget::Compute);        }
    ShaderBuilder& load_tesc(const File& file) { return load_shader(file, ShaderTarget::TessControl);    }
    ShaderBuilder& load_tese(const File& file) { return load_shader(file, ShaderTarget::TessEvaluation); }

    ShaderBuilder& add_shader(const ShaderSource& source, ShaderTarget type)
    {
        shaders_.emplace_back(source, type);
        return *this;
    }

    ShaderBuilder& add_frag(const ShaderSource& source) { return add_shader(source, ShaderTarget::Fragment);       }
    ShaderBuilder& add_vert(const ShaderSource& source) { return add_shader(source, ShaderTarget::Vertex);         }
    ShaderBuilder& add_geom(const ShaderSource& source) { return add_shader(source, ShaderTarget::Geometry);       }
    ShaderBuilder& add_comp(const ShaderSource& source) { return add_shader(source, ShaderTarget::Compute);        }
    ShaderBuilder& add_tesc(const ShaderSource& source) { return add_shader(source, ShaderTarget::TessControl);    }
    ShaderBuilder& add_tese(const ShaderSource& source) { return add_shader(source, ShaderTarget::TessEvaluation); }

    ShaderBuilder& define(String name)
    {
        return define(MOVE(name), "1");
    }

    ShaderBuilder& define(String name, String value)
    {
        defines_.emplace_back(MOVE(name), MOVE(value));
        return *this;
    }

    // Compile and link the program from the specified shaders.
    [[nodiscard]] UniqueProgram get();

private:
    struct UnevaluatedShader
    {
        ShaderSource  source;
        ShaderTarget  type;
        Path          path; // Can be empty if the shader was added, not loaded.
        HashSet<Path> included;

        void resolve_includes();
    };

    struct ShaderDefine
    {
        String name;
        String value;

        auto get_define_string() const -> String
        {
            return "#define " + name + " " + value;
        }
    };

    Vector<UnevaluatedShader> shaders_;
    Vector<ShaderDefine>      defines_;
};


} // namespace josh
