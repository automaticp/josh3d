

# Main interface target of josh3d.
add_library(josh3d INTERFACE)
add_library(josh3d::josh3d ALIAS josh3d)

# Optional ASAN target.
if (JOSH3D_BUILD_WITH_ASAN)
    add_library(josh3d-asan INTERFACE)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(josh3d-asan INTERFACE "-fsanitize=address")
        target_link_options   (josh3d-asan INTERFACE "-fsanitize=address")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(josh3d-asan INTERFACE "/fsanitize=address")
        target_link_options   (josh3d-asan INTERFACE "/fsanitize=address")
    else()
        message(WARNING "Unknown AddressSanitizer support for C++ compiler: ${CMAKE_CXX_COMPILER_ID}.")
        message(WARNING "\"-fsanitize=address\" flag will be passed to the compiler anyway. This might break the build.")
        target_compile_options(josh3d-asan INTERFACE "-fsanitize=address")
        target_link_options   (josh3d-asan INTERFACE "-fsanitize=address")
    endif()
endif()

# Optional TSAN target
if (JOSH3D_BUILD_WITH_TSAN)
    add_library(josh3d-tsan INTERFACE)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(josh3d-tsan INTERFACE "-fsanitize=thread")
        target_link_options   (josh3d-tsan INTERFACE "-fsanitize=thread")
    else()
        message(WARNING "Unknown ThreadSanitizer support for C++ compiler: ${CMAKE_CXX_COMPILER_ID}.")
        message(WARNING "\"-fsanitize=thread\" flag will be passed to the compiler anyway. This might break the build.")
        target_compile_options(josh3d-tsan INTERFACE "-fsanitize=thread")
        target_link_options   (josh3d-tsan INTERFACE "-fsanitize=thread")
    endif()
endif()




set(SUBTARGETS
    data
    engine
    filesystem
    gl
    gltraits
    imgui
    input
    layout
    render
    resource
    scene
    util
)


function(make_subtarget SUBTARGET)

    set(TARGET "josh3d-${SUBTARGET}")
    file(GLOB_RECURSE LOCAL_SOURCES CONFIGURE_DEPENDS "${SUBTARGET}/*.cpp")

    if (LOCAL_SOURCES)
        add_library(${TARGET} STATIC)
        target_sources(${TARGET} PRIVATE "${LOCAL_SOURCES}")
        set(VISIBILITY PUBLIC)
    else()
        add_library(${TARGET} INTERFACE)
        set(VISIBILITY INTERFACE)
    endif()

    target_include_directories(${TARGET} ${VISIBILITY} ${SUBTARGET})
    target_compile_features   (${TARGET} ${VISIBILITY} cxx_std_20)

    if (JOSH3D_BUILD_WITH_ASAN)
        target_link_libraries(${TARGET} ${VISIBILITY} josh3d-asan)
    endif()

    if (JOSH3D_BUILD_WITH_TSAN)
        target_link_libraries(${TARGET} ${VISIBILITY} josh3d-tsan)
    endif()

endfunction()





foreach(SUBTARGET IN ITEMS ${SUBTARGETS})
    make_subtarget(${SUBTARGET})
    target_link_libraries(josh3d INTERFACE josh3d-${SUBTARGET})
endforeach()




# Resolve internal dependencies.
# (Using short names, prefixes each subtarget with "josh3d-")


function(subtarget_link_internal SUBTARGET)
    set(ACCESS_MODS PUBLIC PRIVATE INTERFACE)
    cmake_parse_arguments(ARG "" "" "${ACCESS_MODS}" ${ARGN})

    message(STATUS "${SUBTARGET} -> ${ARGN}")

    foreach(ACCESS IN ITEMS ${ACCESS_MODS})
        foreach(DEP IN ITEMS ${ARG_${ACCESS}})
            target_link_libraries(josh3d-${SUBTARGET} ${ACCESS} josh3d-${DEP})
        endforeach()
    endforeach()

endfunction()


subtarget_link_internal(imgui      PUBLIC engine filesystem input util PRIVATE data gl scene resource)
subtarget_link_internal(engine     PUBLIC data filesystem gl gltraits render resource scene util)
subtarget_link_internal(render     PUBLIC data gl gltraits util layout)
subtarget_link_internal(resource   PUBLIC data filesystem gl gltraits scene util layout PRIVATE render)
subtarget_link_internal(data       PUBLIC util layout)
subtarget_link_internal(input      PUBLIC util scene)
subtarget_link_internal(gl         PUBLIC util layout)
subtarget_link_internal(gltraits   INTERFACE gl layout)
subtarget_link_internal(scene      PUBLIC util layout)
subtarget_link_internal(filesystem PUBLIC util)
subtarget_link_internal(layout)
subtarget_link_internal(util)




# Common external dependencies.
set(common_external_deps
    Boost::container
    Boost::iostreams
    Boost::scope
    Boost::outcome
    cppcoro::cppcoro
    glm::glm
    fmt::fmt
    range-v3::range-v3
    range-v3::concepts
)


function(subtarget_link_common_external_deps SUBTARGET)

    get_target_property(target_type josh3d-${SUBTARGET} TYPE)
    if (target_type STREQUAL "INTERFACE_LIBRARY")
        set(VISIBILITY INTERFACE)
    else()
        set(VISIBILITY PUBLIC)
    endif()

    target_link_libraries(josh3d-${SUBTARGET} ${VISIBILITY} ${common_external_deps})

endfunction()


foreach(SUBTARGET IN ITEMS ${SUBTARGETS})
    subtarget_link_common_external_deps(${SUBTARGET})
endforeach()


target_link_libraries(josh3d-data     PRIVATE   ctre::ctre)
target_link_libraries(josh3d-engine   PUBLIC    glfwpp::glfwpp EnTT::EnTT)
target_link_libraries(josh3d-gl       PUBLIC    glbinding::glbinding glbinding::glbinding-aux)
target_link_libraries(josh3d-imgui    PUBLIC    imgui::imgui unofficial::imguizmo::imguizmo PRIVATE glfwpp::glfwpp)
target_link_libraries(josh3d-resource PUBLIC    assimp::assimp #[[ should be private ]] PRIVATE stb::stb nlohmann_json::nlohmann_json)
# target_link_libraries(josh3d-render)
# target_link_libraries(josh3d-layout)
target_link_libraries(josh3d-scene    PUBLIC    EnTT::EnTT)
target_link_libraries(josh3d-util     PUBLIC    glfwpp::glfwpp #[[ should be in engine ]])

