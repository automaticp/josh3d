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
    globals
    imgui
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


function(resolve_internal_and_common_external_deps SUBTARGET IDEPS)

    get_target_property(target_type ${SUBTARGET} TYPE)
    if (target_type STREQUAL "INTERFACE_LIBRARY")
        set(VISIBILITY INTERFACE)
    else()
        set(VISIBILITY PUBLIC)
    endif()

    message(DEBUG "${SUBTARGET} IDEPS: ${IDEPS}")

    foreach(IDEP IN ITEMS ${IDEPS})
        target_link_libraries(${SUBTARGET} ${VISIBILITY} josh3d-${IDEP})
    endforeach()

    target_link_libraries(${SUBTARGET} ${VISIBILITY} ${common_EDEPS})

endfunction()


# Internal DEPendencieS:
set(data_IDEPS       filesystem util)
set(engine_IDEPS     data filesystem gl globals render)
set(filesystem_IDEPS util)
set(gl_IDEPS         util render #[[ removable, belongs to gl ]])
set(globals_IDEPS    data filesystem gl render resource scene util)
set(imgui_IDEPS      data engine filesystem gl globals scene resource util)
set(render_IDEPS     data filesystem #[[ removable ]] gl resource #[[ removable ]] util)
set(resource_IDEPS   data filesystem gl render #[[ maybe removable ]] util)
set(scene_IDEPS      engine #[[ removable ]] gl globals util)
set(util_IDEPS       scene #[[ removable, needs input target ]] globals)

foreach(DIR IN ITEMS ${SUBTARGET_DIRS})
    resolve_internal_and_common_external_deps(josh3d-${DIR} "${${DIR}_IDEPS}")
endforeach()

target_link_libraries(josh3d-data     PRIVATE   stb::stb)
target_link_libraries(josh3d-engine   PUBLIC    glfwpp::glfwpp EnTT::EnTT)
target_link_libraries(josh3d-gl       PUBLIC    glbinding::glbinding)
target_link_libraries(josh3d-globals  PRIVATE   assimp::assimp)
target_link_libraries(josh3d-imgui    PRIVATE   imgui::imgui glfwpp::glfwpp)
target_link_libraries(josh3d-resource PUBLIC    #[[ should be private ]] assimp::assimp)
target_link_libraries(josh3d-scene    INTERFACE EnTT::EnTT)
target_link_libraries(josh3d-util     INTERFACE glbinding::glbinding glbinding::glbinding-aux glfwpp::glfwpp)
