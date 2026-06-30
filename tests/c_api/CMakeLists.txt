# C-language test for the simdjson C API.
#
# Built deliberately as C (the source is .c): this proves include/simdjson/simdjson_c.h is valid C
# and that the extern "C" symbols link against the C++ library. LINKER_LANGUAGE CXX makes CMake link
# with the C++ driver so libstdc++ is pulled in. This target intentionally does NOT inherit the
# directory-wide C++ warning flags (simdjson-internal-flags), which is why it lives in its own
# subdirectory added before those flags are set.
add_executable(c_api_test c_api_test.c)
set_target_properties(c_api_test PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(c_api_test PRIVATE simdjson)

add_test(NAME c_api_test COMMAND c_api_test)
set_property(TEST c_api_test APPEND PROPERTY LABELS other acceptance)

# Same test forced onto the always-available fallback backend, proving the C layer dispatches
# correctly to a specific separately-compiled object.
add_test(NAME c_api_test_fallback COMMAND c_api_test)
set_tests_properties(c_api_test_fallback PROPERTIES
  ENVIRONMENT "SIMDJSON_FORCE_IMPLEMENTATION=fallback")
set_property(TEST c_api_test_fallback APPEND PROPERTY LABELS other acceptance)
