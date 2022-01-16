add_library(types INTERFACE)
target_sources(types INTERFACE TypeAliases.h LightCasters.h)
target_include_directories(types INTERFACE .)
