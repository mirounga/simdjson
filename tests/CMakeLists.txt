# C API test, added before the directory-wide link_libraries below so it does not inherit the
# C++-only internal flags (it is compiled as C). It links simdjson explicitly.
add_subdirectory(c_api)

# Header-only acceptance test. Created BEFORE the directory-wide link_libraries() calls below so it
# inherits NONE of them: it links ONLY simdjson::simdjson_header_only (headers + SIMDJSON_HEADER_ONLY=1),
# proving a header-only consumer needs no library. Two variants exercise both code paths:
#   - default -march  -> builtin resolves to the baseline backend (portable std::simd shim)
#   - -march=native   -> builtin resolves to the host's native SIMD backend
# NOT gated on SIMDJSON_STATIC_REFLECTION: these tests use only ondemand/dom/minify (no reflection),
# and header-only mode compiles without reflection at C++26 -- this is the default developer build
# where the C++-standard contract of the interface target matters most.
if(SIMDJSON_DEVELOPER_MODE)
  add_executable(header_only_acceptance header_only_acceptance.cpp)
  target_link_libraries(header_only_acceptance PRIVATE simdjson::simdjson_header_only)
  add_test(NAME header_only_acceptance COMMAND header_only_acceptance)
  set_property(TEST header_only_acceptance APPEND PROPERTY LABELS other acceptance header_only)

  add_executable(header_only_acceptance_native header_only_acceptance.cpp)
  target_link_libraries(header_only_acceptance_native PRIVATE simdjson::simdjson_header_only)
  target_compile_options(header_only_acceptance_native PRIVATE -march=native)
  add_test(NAME header_only_acceptance_native COMMAND header_only_acceptance_native)
  set_property(TEST header_only_acceptance_native APPEND PROPERTY LABELS other acceptance header_only)
endif()

# Most tests need test data, and many need windows headers.
link_libraries(simdjson-internal-flags test-data simdjson-windows-headers)
include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)
add_subdirectory(dom)
add_subdirectory(ondemand)

# compilation_failure_tests is added before the global link_libraries(simdjson) so that
# multiple_include/myexe does not receive a duplicate simdjson entry alongside the one
# already propagated by mylib PUBLIC simdjson::simdjson.
add_subdirectory(compilation_failure_tests)

# All remaining tests link with simdjson proper
link_libraries(simdjson)

add_cpp_test(unicode_tests       LABELS dom acceptance per_implementation)
add_cpp_test(minify_tests        LABELS other acceptance per_implementation)
add_cpp_test(padded_string_tests LABELS other acceptance                   )
add_cpp_test(memory_map_tests    LABELS other acceptance                   )
add_cpp_test(prettify_tests      LABELS other acceptance per_implementation)
add_cpp_test(fractured_json_tests LABELS other acceptance per_implementation)

if(WIN32 AND BUILD_SHARED_LIBS)
  # Copy the simdjson dll into the tests directory
  add_custom_command(TARGET unicode_tests POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdjson>"      # <--this is in-file
        "$<TARGET_FILE_DIR:unicode_tests>")                 # <--this is out-file path
endif()

# This tests validates that the implementation is what we think it is if we get passed
# SIMDJSON_FORCE_IMPLEMENTATION, so we know we're testing what we think we're testing
add_cpp_test(checkimplementation LABELS other per_implementation)

add_subdirectory(builder)
add_subdirectory(compile_time)