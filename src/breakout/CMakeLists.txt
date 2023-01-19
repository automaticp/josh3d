add_executable(breakout)
file(GLOB LOCAL_SOURCES CONFIGURE_DEPENDS *.cpp)
target_sources(breakout PRIVATE "${LOCAL_SOURCES}")
target_include_directories(breakout PRIVATE .)
target_compile_features(breakout PRIVATE cxx_std_20)

# internal dependencies
target_link_libraries(breakout
    PRIVATE
        learnlib
)

# external depenencies
target_link_libraries(breakout
    PRIVATE
        glm::glm
        glbinding::glbinding
        glbinding::glbinding-aux
        glfwpp::glfwpp
        range-v3::range-v3
        range-v3::concepts
)
