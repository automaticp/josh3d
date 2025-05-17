#pragma once
#include "Filesystem.hpp"
#include "GLObjects.hpp"
#include "GLShaders.hpp"
#include "GLProgram.hpp"
#include "ReadFile.hpp"
#include "RuntimeError.hpp"
#include "ShaderSource.hpp"
#include <filesystem>
#include <string>
#include <unordered_set>
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


class IncludeResolutionFailure final : public RuntimeError {
public:
    static constexpr auto prefix = "Failed to Resolve Include: ";

    std::string include_name;

    IncludeResolutionFailure(std::string include_name)
        : RuntimeError(prefix, include_name)
        , include_name{ std::move(include_name) }
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
public:
    ShaderBuilder& load_shader(const File& file, ShaderTarget type) {
        shaders_.emplace_back(ShaderSource(read_file(file)), type, file.path());
        shaders_.back().resolve_includes();
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


private:
    struct UnevaluatedShader {
        ShaderSource             source;
        ShaderTarget             type;
        Path                     path; // Can be empty if the shader was added, not loaded.
        std::unordered_set<Path> included;

        void resolve_includes();
    };

    struct ShaderDefine {
        std::string name;
        std::string value;

        std::string get_define_string() const {
            return "#define " + name + " " + value;
        }
    };

    std::vector<UnevaluatedShader> shaders_;
    std::vector<ShaderDefine>      defines_;

};




[[nodiscard]]
inline auto ShaderBuilder::get()
    -> UniqueProgram
{
    UniqueProgram sp;


    auto compile_and_attach =
        [&sp]<typename UniqueShaderType>(
            UniqueShaderType         shader_obj,
            const UnevaluatedShader& shader)
        {
            shader_obj->set_source(shader.source.text_view());
            shader_obj->compile();
            if (!shader_obj->has_compiled_successfully()) {
                throw error::ShaderCompilationFailure(
                    shader.path.string() + '\n' + shader_obj->get_info_log(),
                    UniqueShaderType::target_type
                );
            }
            sp->attach_shader(shader_obj);
        };


    for (auto& shader : shaders_) {

        for (auto& define : defines_) {
            if (std::optional ver_dir = ShaderSource::find_version_directive(shader.source)) {
                shader.source.insert_line_on_line_after(ver_dir->full.begin(), define.get_define_string());
            } else {
                shader.source.insert_line_on_line_before(shader.source.begin(), define.get_define_string());
            }
        }

        switch (shader.type) {
            using enum ShaderTarget;
            case VertexShader:         compile_and_attach(UniqueVertexShader(),         shader); break;
            case FragmentShader:       compile_and_attach(UniqueFragmentShader(),       shader); break;
            case GeometryShader:       compile_and_attach(UniqueGeometryShader(),       shader); break;
            case ComputeShader:        compile_and_attach(UniqueComputeShader(),        shader); break;
            case TessControlShader:    compile_and_attach(UniqueTessControlShader(),    shader); break;
            case TessEvaluationShader: compile_and_attach(UniqueTessEvaluationShader(), shader); break;
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




inline void ShaderBuilder::UnevaluatedShader::resolve_includes() {
    // 1. Get parent directory from path. If there's no path:
    //   - Check that the file does not include anything. Error if it does.
    //   - Skip the rest of the processing.
    // 2. Find include directive and extract relative path.
    // 3. Get canonical path of the include, handle errors.
    // 4. Check if it has already been included, if it has:
    //   - Replace the #include with an empty string.
    //   - Skip to (8).
    // 5. Try to read file of the include, handle errors.
    // 6. Replace the #include with the contents of the new file.
    // 7. Add canonical path of the new include to the "included set".
    // 8. Repeat from (2) unitl no more #includes are found.

    if (path.empty()) {
        if (std::optional include_dir = ShaderSource::find_include_directive(source)) {
            throw error::IncludeResolutionFailure(include_dir->quoted_path.to_string());
        } else {
            return;
        }
    }

    Path parent_dir = path.parent_path();

    while (std::optional include_dir = ShaderSource::find_include_directive(source)) {

        const Path relative_path{ include_dir->path.view() };
        const Path canonical_path = std::filesystem::canonical(parent_dir / relative_path); // Can fail.

        if (included.contains(canonical_path)) {
            // Just erase the #include line.
            source.remove_subrange(include_dir->full);
        } else {
            ShaderSource included_contents{ read_file(File(canonical_path)) }; // Can fail.
            source.replace_subrange(include_dir->full, included_contents);
            included.emplace(canonical_path);
        }
    }
}



} // namespace josh
