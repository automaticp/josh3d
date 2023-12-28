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
    target_compile_features(${TARGET} ${VISIBILITY} cxx_std_20)

endfunction()


set(SUBTARGETS
    data
    engine
    filesystem
    gl
    imgui
    input
    layout
    render
    resource
    scene
    terrain
    util
)


# Main interface target of josh3d.
add_library(josh3d INTERFACE)
add_library(josh3d::josh3d ALIAS josh3d)

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


subtarget_link_internal(
    imgui
        PUBLIC
            engine filesystem input util
        PRIVATE
            data gl scene resource
)
subtarget_link_internal(
    engine
        PUBLIC
            data filesystem gl render resource scene terrain util
)
subtarget_link_internal(
    terrain
        PUBLIC
            data gl scene resource util layout imgui #[[ well... ]]
)
subtarget_link_internal(
    render
        INTERFACE
            data gl util layout
)
subtarget_link_internal(
    resource
        PUBLIC
            data filesystem gl scene util layout
        PRIVATE
            render
)
subtarget_link_internal(
    data
        INTERFACE
            filesystem #[[ ShaderSource can probably have from_file moved to resource ]] util layout
)
subtarget_link_internal(
    input
        INTERFACE
            util scene
)
subtarget_link_internal(
    gl
        PUBLIC
            util layout
)
subtarget_link_internal(
    scene
        INTERFACE
            util layout
)
subtarget_link_internal(
    filesystem
        PUBLIC
            util
)
subtarget_link_internal(
    layout
)
subtarget_link_internal(
    util
)


# Common external dependencies.
set(common_external_deps glm::glm range-v3::range-v3 range-v3::concepts)

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


# target_link_libraries(josh3d-data)
target_link_libraries(josh3d-engine   PUBLIC    glfwpp::glfwpp EnTT::EnTT)
target_link_libraries(josh3d-gl       PUBLIC    glbinding::glbinding)
target_link_libraries(josh3d-imgui    PUBLIC    imgui::imgui PRIVATE glfwpp::glfwpp)
target_link_libraries(josh3d-resource
    PUBLIC assimp::assimp #[[ should be private ]]
    PRIVATE stb::stb nlohmann_json::nlohmann_json
)
# target_link_libraries(josh3d-render)
# target_link_libraries(josh3d-layout)
target_link_libraries(josh3d-scene    INTERFACE EnTT::EnTT)
target_link_libraries(josh3d-util     PUBLIC    glbinding::glbinding glbinding::glbinding-aux glfwpp::glfwpp)

