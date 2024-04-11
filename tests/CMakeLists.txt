add_executable(josh3d-tests)
file(GLOB LOCAL_SOURCES CONFIGURE_DEPENDS ./josh3d/*.cpp)
target_sources            (josh3d-tests PRIVATE "${LOCAL_SOURCES}")
target_include_directories(josh3d-tests PRIVATE ./josh3d)
target_compile_features   (josh3d-tests PRIVATE cxx_std_20)


target_link_libraries(josh3d-tests PRIVATE josh3d doctest::doctest range-v3::range-v3 range-v3::concepts)
