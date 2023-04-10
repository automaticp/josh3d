add_executable(learnlib-tests)
file(GLOB LOCAL_SOURCES CONFIGURE_DEPENDS ./learnlib/*.cpp)
target_sources(learnlib-tests PRIVATE "${LOCAL_SOURCES}")
target_include_directories(learnlib-tests PRIVATE ./learnlib)
target_compile_features(learnlib-tests PRIVATE cxx_std_20)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(learnlib-tests PRIVATE -fsanitize=thread)
    target_link_options(learnlib-tests PRIVATE -fsanitize=thread)
endif()


target_link_libraries(learnlib-tests PRIVATE learnlib doctest::doctest range-v3::range-v3 range-v3::concepts)
