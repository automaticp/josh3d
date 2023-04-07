add_executable(breakout2)
file(GLOB LOCAL_SOURCES CONFIGURE_DEPENDS *.cpp)
target_sources(breakout2 PRIVATE "${LOCAL_SOURCES}")
target_include_directories(breakout2 PRIVATE .)
target_compile_features(breakout2 PRIVATE cxx_std_20)

# internal dependencies
target_link_libraries(breakout2
    PRIVATE
        learnlib
)

# external depenencies
target_link_libraries(breakout2
    PRIVATE
        glm::glm
        glbinding::glbinding
        glbinding::glbinding-aux
        glfwpp::glfwpp
        range-v3::range-v3
        range-v3::concepts
        EnTT::EnTT
        box2d::box2d
        imgui::imgui
)
