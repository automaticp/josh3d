
find_package(glbinding CONFIG REQUIRED)

set_target_properties(glbinding::glbinding PROPERTIES IMPORTED_GLOBAL TRUE)
set_target_properties(glbinding::glbinding-aux PROPERTIES IMPORTED_GLOBAL TRUE)
