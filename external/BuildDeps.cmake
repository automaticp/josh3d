# NOTE: Because this is included as a script, all directories
# have to be explicit and not relative to the script directory.

# === dear-imgui ===
# Targets:
#   imgui::imgui
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
add_library(dear-imgui STATIC)
target_sources(dear-imgui PRIVATE
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(dear-imgui PUBLIC
    ${IMGUI_DIR}/
    ${IMGUI_DIR}/misc/cpp/
    ${IMGUI_DIR}/backends/)
add_library(imgui::imgui ALIAS dear-imgui)
