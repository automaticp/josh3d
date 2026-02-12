# Symlinks the compile_commands.json to root for the LSP to pick up
# the latest configuration. This helps in dev when switching configs.

file(CREATE_LINK
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
    SYMBOLIC)

message(STATUS "Symlinked ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json to root.")
