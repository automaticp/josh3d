#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <glfwpp/glfwpp.h>
#include <cxxopts.hpp> // This lib is kinda garbage from the POV of C++ idiomatics, replace later
#include <boost/iostreams/tee.hpp>
#include <boost/iostreams/stream.hpp>
#include <optional>
#include <iostream>
#include <vector>

#include "DemoScene.hpp"
#include "Filesystem.hpp"
#include "GlobalContext.hpp"
#include "Logging.hpp"
#include "GLUtils.hpp"
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
            "log-gl-errors",
            "Enable logging of OpenGL errors",
            cxxopts::value<bool>()->default_value("true")
        )
        (
            "log-gl-shaders",
            "Enable logging of OpenGL shader compilation/linking",
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

    auto cli_options      = get_cli_options();
    auto cli_parse_result = try_parse_cli_args(cli_options, argc, argv);

    if (!cli_parse_result.has_value()) {
        return 1;
    }


    auto& cli_args = cli_parse_result.value();

    if (cli_args.count("help")) {
        std::cout << cli_options.help() << "\n";
        return 0;
    }

    if (cli_args.count("vroots")) {
        auto vroot_strigns = cli_args["vroots"].as<std::vector<std::string>>();

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

    glfw::Window window{ 1280, 720, "Josh3D Demo" };
    glfw::makeContextCurrent(window);
    glfw::swapInterval(0);
    window.setInputModeCursor(glfw::CursorMode::Normal);


    glbinding::initialize(glfwGetProcAddress);


    josh::globals::RAIIContext globals_context;
    josh::globals::window_size.track(window);

    auto hook_gl_logs_to = [&](std::ostream& os) {
        if (cli_args["log-gl-errors"].as<bool>()) {
            josh::log_gl_errors(os);
        }
        if (cli_args["log-gl-shaders"].as<bool>()) {
            josh::log_gl_shader_creation(os);
        }
    };


    hook_gl_logs_to(josh::logstream()); // Hook now to report initialization failures.


    DemoScene application{ window };

    // The initialization will only log to the previously set logstream.
    // Everything after will tee to the ImGui log window.
    auto tee_device  = boost::iostreams::tee(std::clog, application.get_log_sink());
    auto tee_ostream = boost::iostreams::stream(tee_device);
    josh::set_logstream(tee_ostream);

    hook_gl_logs_to(josh::logstream()); // Now re-hook to a tee between console and ImGui window.


    while (!application.is_done()) {
        application.execute_frame();
        tee_ostream.flush(); // Does not auto flush, do it manually every frame.
    }

    return 0;
}


