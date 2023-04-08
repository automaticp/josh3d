add_executable(tests-learnlib)
file(GLOB LOCAL_SOURCES CONFIGURE_DEPENDS ./learnlib/*.cpp)
target_sources(tests-learnlib PRIVATE "${LOCAL_SOURCES}")
target_include_directories(tests-learnlib PUBLIC ./learnlib)
target_compile_features(tests-learnlib PUBLIC cxx_std_20)

target_link_libraries(tests-learnlib PRIVATE learnlib doctest::doctest range-v3::range-v3 range-v3::concepts)
