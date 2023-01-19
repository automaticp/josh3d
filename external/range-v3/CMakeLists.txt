
find_package(range-v3 CONFIG REQUIRED)

set_target_properties(range-v3::range-v3 PROPERTIES IMPORTED_GLOBAL TRUE)
set_target_properties(range-v3::concepts PROPERTIES IMPORTED_GLOBAL TRUE)
