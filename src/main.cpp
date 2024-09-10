#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <glfwpp/glfwpp.h>
#include <cxxopts.hpp> // This lib is kinda garbage from the POV of C++ idiomatics, replace later
#include <optional>
#include <vector>

#include "DemoScene.hpp"
#include "Filesystem.hpp"
#include "GlobalContext.hpp"
#include "Logging.hpp"
#include "ShaderPool.hpp"
#include "VirtualFilesystem.hpp"
#include "WindowSizeCache.hpp"




static auto get_cli_options()
    -> cxxopts::Options
{
    cxxopts::Options options{ "josh3d-demo", "A demo application for the josh3d rendering engine." };

    options.add_options()
        (
            "vroots",
            "Root directories for the VFS to use as base when resolving Virtual Paths",
            cxxopts::value<std::vector<std::string>>()
        )
        (
            "enable-gl-logging",
            "Enable logging of: object creation/destruction OpenGL calls, OpenGL errors, etc.",
            cxxopts::value<bool>()->default_value("true")
        )
        (
            "h,help",
            "Print help and exit"
        )
    ;

    return options;
}


static auto try_parse_cli_args(cxxopts::Options& options, int argc, const char* argv[])
    -> std::optional<cxxopts::ParseResult>
{
    try {
        return options.parse(argc, argv);
    } catch (const cxxopts::exceptions::parsing& e) {
        std::cerr
            << e.what()       << "\n"
            << options.help() << "\n";
            return std::nullopt;
    }
}


int main(int argc, const char* argv[]) {

    auto cli_options = get_cli_options();

    auto cli_parse_result = try_parse_cli_args(cli_options, argc, argv);

    if (!cli_parse_result.has_value()) {
        return 1;
    }

    if (cli_parse_result->count("help")) {
        std::cout << cli_options.help() << "\n";
        return 0;
    }

    if (cli_parse_result->count("vroots")) {
        auto vroot_strigns = (*cli_parse_result)["vroots"].as<std::vector<std::string>>();

        try {
            for (auto&& vroot : vroot_strigns) {
                josh::vfs().roots().push_front(josh::Directory(std::move(vroot)));
            }
        } catch (const josh::error::DirectoryDoesNotExist& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
    }


    auto glfw_instance{ glfw::init() };

    glfw::WindowHints{
        .scaleToMonitor = true,
        .srgbCapable    = true,
        .contextVersionMajor = 4,
        .contextVersionMinor = 6,
        .openglProfile = glfw::OpenGlProfile::Core,
    }.apply();

    glfw::Window window{ 1280, 720, "Josh3d Demo" };
    glfw::makeContextCurrent(window);
    glfw::swapInterval(0);
    window.setInputModeCursor(glfw::CursorMode::Normal);


    glbinding::initialize(glfwGetProcAddress);

    if ((*cli_parse_result)["enable-gl-logging"].as<bool>()) {
        josh::enable_glbinding_logger();
    }


    josh::globals::RAIIContext globals_context;
    josh::globals::window_size.track(window);


    DemoScene application{ window };

    application.applicate();


    return 0;
}


