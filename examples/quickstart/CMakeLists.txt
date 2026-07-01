#
# Quick-start drop-in tests.
#
# The old model compiled src/simdjson.cpp together with a C++ example at C++11/14. That no longer
# applies: the SIMD kernel is C++26 std::simd, and the backends are separately-compiled objects in
# the library. So the drop-in story is now two-tier:
#   * below-C++26 / any-language consumers -> the extern "C" API + the prebuilt dispatched library
#   * C++26 consumers                      -> the header-only On-Demand API + the library
# Both link the real `simdjson` target, which is exactly how a downstream project consumes it.
#
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

  # ---- C API drop-in (compiles as C, links the dispatched library) ----
  add_executable(quickstart_c quickstart_c.c)
  # Link with the C++ driver so libstdc++ is pulled in for the C++ library.
  set_target_properties(quickstart_c PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(quickstart_c PRIVATE simdjson)
  add_test(NAME quickstart_c COMMAND quickstart_c)
  set_property(TEST quickstart_c APPEND PROPERTY LABELS quickstart acceptance)

  # Same test forced onto the always-available fallback backend, proving runtime dispatch.
  add_test(NAME quickstart_c_fallback COMMAND quickstart_c)
  set_tests_properties(quickstart_c_fallback PROPERTIES
    ENVIRONMENT "SIMDJSON_FORCE_IMPLEMENTATION=fallback")
  set_property(TEST quickstart_c_fallback APPEND PROPERTY LABELS quickstart acceptance)

  # ---- C++26 header-only drop-in (On-Demand) ----
  if (SIMDJSON_EXCEPTIONS)
    add_executable(quickstart_cpp quickstart_cpp.cpp)
    target_link_libraries(quickstart_cpp PRIVATE simdjson)
    add_test(NAME quickstart_cpp COMMAND quickstart_cpp)
    set_property(TEST quickstart_cpp APPEND PROPERTY LABELS quickstart acceptance quickstart_ondemand)
  endif()

endif()
