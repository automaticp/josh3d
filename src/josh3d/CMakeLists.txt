function(make_subtarget SUBTARGET_DIR)

    set(SUBTARGET "josh3d-${SUBTARGET_DIR}")
    file(GLOB_RECURSE LOCAL_SOURCES CONFIGURE_DEPENDS "${SUBTARGET_DIR}/*.cpp")

    if (LOCAL_SOURCES)
        add_library(${SUBTARGET} STATIC)
        target_sources(${SUBTARGET} PRIVATE "${LOCAL_SOURCES}")
        set(VISIBILITY PUBLIC)
    else()
        add_library(${SUBTARGET} INTERFACE)
        set(VISIBILITY INTERFACE)
    endif()

    target_include_directories(${SUBTARGET} ${VISIBILITY} ${SUBTARGET_DIR})
    target_compile_features(${SUBTARGET} ${VISIBILITY} cxx_std_20)

endfunction()


set(SUBTARGET_DIRS
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
    util
)


# Main interface target of josh3d.
add_library(josh3d INTERFACE)
add_library(josh3d::josh3d ALIAS josh3d)

foreach(DIR IN ITEMS ${SUBTARGET_DIRS})
    make_subtarget(${DIR})
    target_link_libraries(josh3d INTERFACE josh3d-${DIR})
endforeach()




# Common external dependencies.
set(common_EDEPS glm::glm range-v3::range-v3 range-v3::concepts)


function(resolve_internal_and_common_external_deps SUBTARGET IDEPS PDEPS)

    get_target_property(target_type ${SUBTARGET} TYPE)
    if (target_type STREQUAL "INTERFACE_LIBRARY")
        set(VISIBILITY INTERFACE)
    else()
        set(VISIBILITY PUBLIC)
    endif()

    message(STATUS "${SUBTARGET} IDEPS: ${IDEPS}")
    message(STATUS "${SUBTARGET} PDEPS: ${PDEPS}")

    foreach(IDEP IN ITEMS ${IDEPS})
        target_link_libraries(${SUBTARGET} ${VISIBILITY} josh3d-${IDEP})
    endforeach()


    foreach(PDEP IN ITEMS ${PDEPS})
        target_link_libraries(${SUBTARGET} PRIVATE josh3d-${PDEP})
    endforeach()


    target_link_libraries(${SUBTARGET} ${VISIBILITY} ${common_EDEPS})

endfunction()


# [Internal DEPendencieS] and [internal Private DEPendencieS]:
set(imgui_IDEPS      engine filesystem input util)
set(imgui_PDEPS      data gl scene resource)
set(engine_IDEPS     data filesystem gl render resource scene)
set(engine_PDEPS     )
set(render_IDEPS     data filesystem #[[ removable VPath ]] resource #[[ removable ShaderBuilder ]] gl util)
set(render_PDEPS     )
set(resource_IDEPS   data filesystem gl scene util layout)
set(resource_PDEPS   render)
set(data_IDEPS       filesystem util layout)
set(data_PDEPS       )
set(input_IDEPS      util scene)
set(input_PDEPS      )
set(gl_IDEPS         util layout)
set(gl_PDEPS         )
set(scene_IDEPS      util layout)
set(scene_PDEPS      )
set(filesystem_IDEPS util)
set(filesystem_PDEPS )
set(layout_IDEPS     )
set(layout_PDEPS     resource #[[ remove get_vertex_data ]])
set(util_IDEPS       )
set(util_PDEPS       )


foreach(DIR IN ITEMS ${SUBTARGET_DIRS})
    resolve_internal_and_common_external_deps(
        josh3d-${DIR} "${${DIR}_IDEPS}" "${${DIR}_PDEPS}"
    )
endforeach()


target_link_libraries(josh3d-data     PRIVATE   stb::stb assimp::assimp)
target_link_libraries(josh3d-engine   PUBLIC    glfwpp::glfwpp EnTT::EnTT)
target_link_libraries(josh3d-gl       PUBLIC    glbinding::glbinding)
target_link_libraries(josh3d-imgui    PUBLIC    imgui::imgui PRIVATE glfwpp::glfwpp)
target_link_libraries(josh3d-resource PUBLIC    #[[ should be private ]] assimp::assimp)
# target_link_libraries(josh3d-render)
target_link_libraries(josh3d-layout   PRIVATE   assimp::assimp #[[ oh, jesus, it's the vertex data loading ]])
target_link_libraries(josh3d-scene    INTERFACE EnTT::EnTT)
target_link_libraries(josh3d-util     PUBLIC    glbinding::glbinding glbinding::glbinding-aux glfwpp::glfwpp)

