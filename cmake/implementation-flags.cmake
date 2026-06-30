#
# Implementation selection
#
set(SIMDJSON_ALL_IMPLEMENTATIONS fallback westmere haswell icelake arm64 sve ppc64)

set(
    SIMDJSON_IMPLEMENTATION ""
    CACHE STRING "\
Semicolon-separated list of implementations to include \
(${SIMDJSON_ALL_IMPLEMENTATIONS}). If this is not set, any implementations \
that are supported at compile time and may be selected at runtime will be \
included."
)
set(
    SIMDJSON_EXCLUDE_IMPLEMENTATION ""
    CACHE STRING "\
Semicolon-separated list of implementations to exclude \
(icelake/haswell/westmere/arm64/ppc64/fallback). By default, excludes any \
implementations that are unsupported at compile time or cannot be selected at \
runtime."
)

foreach(var IN ITEMS IMPLEMENTATION EXCLUDE_IMPLEMENTATION)
  set(var "SIMDJSON_${var}")
  foreach(impl IN LISTS "${var}")
    if(NOT impl IN_LIST SIMDJSON_ALL_IMPLEMENTATIONS)
      message(ERROR "\
Implementation ${impl} found in ${var} not supported by simdjson. \
Possible implementations: ${SIMDJSON_ALL_IMPLEMENTATIONS}")
    endif()
  endforeach()
endforeach()

macro(flag_action action var val)
  message(STATUS "${action} implementation ${impl} due to ${var}=${${var}}")
  simdjson_add_props(
      target_compile_definitions PUBLIC
      "SIMDJSON_IMPLEMENTATION_${impl_upper}=${val}"
  )
endmacro()

foreach(impl IN LISTS SIMDJSON_ALL_IMPLEMENTATIONS)
  string(TOUPPER "${impl}" impl_upper)
  if(impl IN_LIST SIMDJSON_EXCLUDE_IMPLEMENTATION)
    flag_action(Excluding SIMDJSON_EXCLUDE_IMPLEMENTATION 0)
  elseif(impl IN_LIST SIMDJSON_IMPLEMENTATION)
    flag_action(Including SIMDJSON_IMPLEMENTATION 1)
  elseif(SIMDJSON_IMPLEMENTATION)
    flag_action(Excluding SIMDJSON_IMPLEMENTATION 0)
  endif()
endforeach()

# TODO make it so this generates the necessary compiler flags to select the
# given impl as the builtin automatically!
set(
    SIMDJSON_BUILTIN_IMPLEMENTATION ""
    CACHE STRING "\
Select the implementation that will be used for user code. Defaults to the \
most universal implementation in SIMDJSON_IMPLEMENTATION (in the order \
${SIMDJSON_ALL_IMPLEMENTATIONS}) if specified; otherwise, by default the \
compiler will pick the best implementation that can always be selected given \
the compiler flags."
)
if(NOT SIMDJSON_BUILTIN_IMPLEMENTATION STREQUAL "")
  simdjson_add_props(
      target_compile_definitions PUBLIC
      "SIMDJSON_BUILTIN_IMPLEMENTATION=${SIMDJSON_BUILTIN_IMPLEMENTATION}"
  )
else()
  # Pick the most universal implementation out of the selected implementations
  # (if any)
  foreach(impl IN LISTS SIMDJSON_ALL_IMPLEMENTATIONS)
    if(
        impl IN_LIST SIMDJSON_IMPLEMENTATION
        AND NOT impl IN_LIST SIMDJSON_EXCLUDE_IMPLEMENTATION
    )
      message(STATUS "\
Selected implementation ${impl} as builtin implementation based on \
${SIMDJSON_IMPLEMENTATION}")
      simdjson_add_props(
          target_compile_definitions PUBLIC
          "SIMDJSON_BUILTIN_IMPLEMENTATION=${impl}"
      )
      break()
    endif()
  endforeach()
endif()

foreach(impl IN LISTS SIMDJSON_ALL_IMPLEMENTATIONS)
  string(TOUPPER "${impl}" impl_upper)
  option(
      "SIMDJSON_IMPLEMENTATION_${impl_upper}"
      "Include the ${impl} implementation"
      ON
  )
  mark_as_advanced("SIMDJSON_IMPLEMENTATION_${impl_upper}")
  if(NOT "${SIMDJSON_IMPLEMENTATION_${impl_upper}}")
    message(DEPRECATION "\
SIMDJSON_IMPLEMENTATION_${impl_upper} is deprecated. \
Use SIMDJSON_IMPLEMENTATION=-${impl} instead")
     simdjson_add_props(
        target_compile_definitions PUBLIC
        "SIMDJSON_IMPLEMENTATION_${impl_upper}=0"
    )
  endif()
endforeach()

#
# Separate per-ISA object compilation (replaces the single-TU amalgamation).
#
# The std::simd kernel needs a GLOBAL -march per translation unit (its always_inline
# ops carry no per-function target attribute), so each backend is compiled as its own
# object with its own flags and linked together; the runtime dispatcher in
# src/implementation.cpp picks among them. Here we determine this target arch's
# backends, their per-object flags, and the enabled subset. The library sources and
# per-source flags are wired up in the top-level CMakeLists.txt.
#
# Every known backend (used to force a deterministic, uniform impl set on the library).
set(SIMDJSON_KNOWN_IMPLEMENTATIONS
    fallback westmere haswell icelake arm64 sve ppc64 lsx lasx rvv-vls)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64|i.86|x86")
  set(SIMDJSON_ARCH_BACKENDS fallback westmere haswell icelake)
  set(SIMDJSON_MARCH_fallback "")
  set(SIMDJSON_MARCH_westmere -msse4.2 -mpclmul -mpopcnt)
  set(SIMDJSON_MARCH_haswell  -mavx2 -mbmi -mpclmul -mlzcnt -mpopcnt)
  set(SIMDJSON_MARCH_icelake  -mavx512f -mavx512dq -mavx512cd -mavx512bw
                              -mavx512vbmi -mavx512vbmi2 -mavx512vl
                              -mavx2 -mbmi -mpclmul -mlzcnt -mpopcnt)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
  set(SIMDJSON_ARCH_BACKENDS fallback arm64 sve)
  set(SIMDJSON_MARCH_fallback "")
  set(SIMDJSON_MARCH_arm64 "")                         # NEON is baseline on aarch64
  set(SIMDJSON_MARCH_sve -march=armv9-a+sve2 -msve-vector-bits=512)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "loongarch64")
  set(SIMDJSON_ARCH_BACKENDS fallback lsx lasx)
  set(SIMDJSON_MARCH_fallback "")
  set(SIMDJSON_MARCH_lsx -mlsx)
  set(SIMDJSON_MARCH_lasx -mlasx)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64|powerpc64")
  set(SIMDJSON_ARCH_BACKENDS fallback ppc64)
  set(SIMDJSON_MARCH_fallback "")
  set(SIMDJSON_MARCH_ppc64 -mcpu=power9)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64")
  set(SIMDJSON_ARCH_BACKENDS fallback rvv-vls)
  set(SIMDJSON_MARCH_fallback "")
  set(SIMDJSON_MARCH_rvv_vls -march=rv64gcv -mrvv-vector-bits=zvl)
else()
  set(SIMDJSON_ARCH_BACKENDS fallback)
  set(SIMDJSON_MARCH_fallback "")
endif()

# Enabled subset = arch backends, filtered by SIMDJSON_IMPLEMENTATION / EXCLUDE.
set(SIMDJSON_ENABLED_BACKENDS "")
foreach(impl IN LISTS SIMDJSON_ARCH_BACKENDS)
  if(impl IN_LIST SIMDJSON_EXCLUDE_IMPLEMENTATION)
    # excluded by the user
  elseif(SIMDJSON_IMPLEMENTATION AND NOT impl IN_LIST SIMDJSON_IMPLEMENTATION)
    # an explicit include list was given and this backend is not in it
  else()
    list(APPEND SIMDJSON_ENABLED_BACKENDS "${impl}")
  endif()
endforeach()
message(STATUS "simdjson: per-ISA backends (separate objects): ${SIMDJSON_ENABLED_BACKENDS}")
