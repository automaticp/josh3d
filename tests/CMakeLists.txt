add_executable(learnlib-tests)
file(GLOB LOCAL_SOURCES CONFIGURE_DEPENDS ./learnlib/*.cpp)
target_sources(learnlib-tests PRIVATE "${LOCAL_SOURCES}")
target_include_directories(learnlib-tests PUBLIC ./learnlib)
target_compile_features(learnlib-tests PUBLIC cxx_std_20)

target_link_libraries(learnlib-tests PRIVATE learnlib doctest::doctest range-v3::range-v3 range-v3::concepts)
