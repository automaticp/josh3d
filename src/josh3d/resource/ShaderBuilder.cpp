#include "ShaderBuilder.hpp"
#include "Common.hpp"
#include "GLObjects.hpp"


namespace josh {


auto ShaderBuilder::get()
    -> UniqueProgram
{
    UniqueProgram sp;

    for (auto& shader : shaders_)
    {
        for (auto& define : defines_)
        {
            if (Optional ver_dir = ShaderSource::find_version_directive(shader.source))
                shader.source.insert_line_on_line_after(ver_dir->full.begin(), define.get_define_string());
            else
                shader.source.insert_line_on_line_before(shader.source.begin(), define.get_define_string());
        }

        auto shader_obj = UniqueShader(shader.type);

        shader_obj->set_source(shader.source.text_view());
        shader_obj->compile();

        if (not shader_obj->has_compiled_successfully())
            throw ShaderCompilationFailure(
                shader.path.string(),
                { shader_obj->get_info_log(), shader.type });

        sp->attach_shader(shader_obj);
    }

    sp->link();

    if (not sp->has_linked_successfully())
        throw ProgramLinkingFailure({}, { sp->get_info_log() });

    return sp;
}

void ShaderBuilder::UnevaluatedShader::resolve_includes()
{
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

    if (path.empty())
    {
        if (Optional include_dir = ShaderSource::find_include_directive(source))
            throw IncludeResolutionFailure({}, { .include_name=include_dir->quoted_path.to_string() });
        else
            return;
    }

    Path parent_dir = path.parent_path();

    while (Optional include_dir = ShaderSource::find_include_directive(source))
    {
        const Path relative_path{ include_dir->path.view() };
        const Path canonical_path = std::filesystem::canonical(parent_dir / relative_path); // Can fail.

        if (included.contains(canonical_path))
        {
            // Just erase the #include line.
            source.remove_subrange(include_dir->full);
        }
        else
        {
            ShaderSource included_contents{ read_file(File(canonical_path)) }; // Can fail.
            source.replace_subrange(include_dir->full, included_contents);
            included.emplace(canonical_path);
        }
    }
}


} // namespace josh
