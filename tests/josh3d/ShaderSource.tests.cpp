#include "ShaderSource.hpp"
#include <doctest/doctest.h>
#include <optional>
#include <string_view>


using namespace josh;


TEST_SUITE("ShaderSource") {



TEST_CASE("find_version_directive()") {

    const ShaderSource source{
R"(
  #  version       430       core

  # define TEST 42
    #  extension GL_GOOGLE_include_directive: enable
  #   include "utils.glsl"

    void main() {
        gl_Position = vec4(1.0);
    }
)"
    };

    std::optional ver_dir = source.find_version_directive(source);
    REQUIRE(ver_dir.has_value());
    CHECK(ver_dir->version.view() == "430");
    CHECK(ver_dir->profile.view() == "core");
    CHECK(ver_dir->full.view()    == "  #  version       430       core");

}


TEST_CASE("find_include_extension_directive()") {

    const ShaderSource source{
R"(
  #version 430 core

  # define TEST 42
    #  extension GL_GOOGLE_include_directive: enable
  #   include "utils.glsl"

void main() {
    gl_Position = vec4(1.0);
}
)"
    };

    std::optional ext_dir = source.find_include_extension_directive(source);
    REQUIRE(ext_dir.has_value());
    CHECK(ext_dir->behavior.view() == "enable");
    CHECK(ext_dir->full.view()     == "    #  extension GL_GOOGLE_include_directive: enable");

}


TEST_CASE("find_include_directive()") {
    const ShaderSource source{
R"(
#version 430 core
#extension GL_GOOGLE_include_directive : enable
  #   include "path/to/utils1.glsl"
    #   include <path/to/utils2.glsl>
      #   define TEST "42"
        #   include "path/to/utils3.glsl"

void main() {
    gl_Position = vec4(1.0);
}
)"
    };

    std::optional inc1_dir = source.find_include_directive(source);
    REQUIRE(inc1_dir.has_value());
    CHECK(inc1_dir->path.view()        == "path/to/utils1.glsl");
    CHECK(inc1_dir->quoted_path.view() == "\"path/to/utils1.glsl\"");
    CHECK(inc1_dir->full.view()        == "  #   include \"path/to/utils1.glsl\"");

    std::optional inc2_dir = source.find_include_directive({ inc1_dir->full.end(), source.end() });
    REQUIRE(inc2_dir.has_value());
    CHECK(inc2_dir->path.view()        == "path/to/utils2.glsl");
    CHECK(inc2_dir->quoted_path.view() == "<path/to/utils2.glsl>");
    CHECK(inc2_dir->full.view()        == "    #   include <path/to/utils2.glsl>");

    std::optional inc3_dir = source.find_include_directive({ inc2_dir->full.end(), source.end() });
    REQUIRE(inc3_dir.has_value());
    CHECK(inc3_dir->path.view()        == "path/to/utils3.glsl");
    CHECK(inc3_dir->quoted_path.view() == "\"path/to/utils3.glsl\"");
    CHECK(inc3_dir->full.view()        == "        #   include \"path/to/utils3.glsl\"");

}


TEST_CASE("insert_before()") {

    ShaderSource source{
R"(
#version 430 core
out vec2 uv;
void main() {
// Insert here:
//  v
}
)"
    };

    const std::string_view result{
R"(
#version 430 core
out vec2 uv;
void main() {
// Insert here:
//  v
    uv          = vec2(0.0, 1.0);
    gl_Position = vec4(1.0, 0.0, 1.0, 1.0);
}
)"
    };

    // -1 for newline at the end.
    // -1 for closing brace.
    auto insert_pos = source.end() - 2;
    auto inserted_range = source.insert_before(insert_pos, "    gl_Position = vec4(1.0, 0.0, 1.0, 1.0);\n");
    source.insert_before(inserted_range.begin(),           "    uv          = vec2(0.0, 1.0);\n");

    CHECK(source == result);
}


TEST_CASE("insert_after()") {

    ShaderSource source{
R"(
#version 430 core
out vec2 uv;
void main() {
// Insert here:
//  v
}
)"
    };

    const std::string_view result{
R"(
#version 430 core
out vec2 uv;
void main() {
// Insert here:
//  v
    uv          = vec2(0.0, 1.0);
    gl_Position = vec4(1.0, 0.0, 1.0, 1.0);
}
)"
    };

    // -1 for newline at the end.
    // -1 for closing brace.
    // -1 for newline after "//  v" line.
    auto insert_pos = source.end() - 3;
    auto inserted_range = source.insert_after(insert_pos, "    uv          = vec2(0.0, 1.0);\n");
    source.insert_after(inserted_range.end() - 1,         "    gl_Position = vec4(1.0, 0.0, 1.0, 1.0);\n");

    CHECK(source == result);
}


TEST_CASE("insert_line_on_line_after()") {

    ShaderSource source{
R"(
#version 430 core
#define HELLO 12
#extension GL_GOOGLE_include_directive : enable
#include "path/to/utils1.glsl"
)"
    };

    const std::string_view result{
R"(
#version 430 core
#define TEST 42
#define HELLO 12
#extension GL_GOOGLE_include_directive : enable
#include "path/to/utils1.glsl"
#define MAX_TRIANGLES 10
)"
    };

    {
        std::optional ver_dir = source.find_version_directive(source);
        REQUIRE(ver_dir.has_value());
        // Point to some arbitrary char on that line.
        auto inserted_range = source.insert_line_on_line_after(ver_dir->version.begin(), "#define TEST 42");
        CHECK(inserted_range.view() == "#define TEST 42\n");
    }

    {
        std::optional inc_dir = source.find_include_directive(source);
        REQUIRE(inc_dir.has_value());
        auto inserted_range = source.insert_line_on_line_after(inc_dir->path.begin(), "#define MAX_TRIANGLES 10");
        CHECK(inserted_range.view() == "#define MAX_TRIANGLES 10\n");
    }

    CHECK(source == result);
}


TEST_CASE("insert_line_on_line_before()") {

    ShaderSource source{
R"(
#version 430 core
)"
    };

    const std::string_view result{
R"(
#define TEST 42
#version 430 core
)"
    };

    std::optional ver_dir = source.find_version_directive(source);
    REQUIRE(ver_dir.has_value());
    source.insert_line_on_line_before(ver_dir->full.begin(), "#define TEST 42");

    CHECK(source == result);
}


TEST_CASE("insertions around EOF") {

    ShaderSource source{
    // Line does not end in a newline.
R"(#version 430 core)"
    };
    const std::string_view result{
R"(// This is a comment.
#version 430 core
void main() { gl_Position = vec4(1.0); }
)"
    };

    source.insert_line_on_line_after(source.begin(), "void main() { gl_Position = vec4(1.0); }");
    source.insert_line_on_line_before(source.begin(), "// This is a comment.");

    CHECK(source == result);
}


} // TEST_SUITE("ShaderSource")
