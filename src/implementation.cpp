#ifndef SIMDJSON_SRC_IMPLEMENTATION_CPP
#define SIMDJSON_SRC_IMPLEMENTATION_CPP

#include <base.h>
#include <simdjson/implementation.h>
// builtin_implementation() below needs SIMDJSON_BUILTIN_IMPLEMENTATION; include its definition
// directly rather than relying on a transitive include (previously pulled in via the now-removed
// src/internal/*_tables.cpp which included implementation_detection.h).
#include <simdjson/implementation_detection.h>
// --- isadetection.h merged inline (its only consumer was this TU; src/internal removed) ---
/* From
https://github.com/endorno/pytorch/blob/master/torch/lib/TH/generic/simd/simd.h
Highly modified.

Copyright (c) 2016-     Facebook, Inc            (Adam Paszke)
Copyright (c) 2014-     Facebook, Inc            (Soumith Chintala)
Copyright (c) 2011-2014 Idiap Research Institute (Ronan Collobert)
Copyright (c) 2012-2014 Deepmind Technologies    (Koray Kavukcuoglu)
Copyright (c) 2011-2012 NEC Laboratories America (Koray Kavukcuoglu)
Copyright (c) 2011-2013 NYU                      (Clement Farabet)
Copyright (c) 2006-2010 NEC Laboratories America (Ronan Collobert, Leon Bottou,
Iain Melvin, Jason Weston) Copyright (c) 2006      Idiap Research Institute
(Samy Bengio) Copyright (c) 2001-2004 Idiap Research Institute (Ronan Collobert,
Samy Bengio, Johnny Mariethoz)

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the names of Facebook, Deepmind Technologies, NYU, NEC Laboratories
America and IDIAP Research Institute nor the names of its contributors may be
   used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


#include "simdjson/internal/instruction_set.h"

#include <cstdint>
#include <cstdlib>
#if defined(_MSC_VER)
#include <intrin.h>
#elif (defined(HAVE_GCC_GET_CPUID) && defined(USE_GCC_GET_CPUID)) || defined(__FILC__)
#include <cpuid.h>
#endif
#if defined(__loongarch__) && defined(__linux__)
  #include <sys/auxv.h>
#endif

#ifdef __FILC__
#include <stdfil.h>
#endif

namespace simdjson {
namespace internal {

#if defined(__PPC64__)

static inline uint32_t detect_supported_architectures() {
  return instruction_set::ALTIVEC;
}

#elif defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)

static inline uint32_t detect_supported_architectures() {
  return instruction_set::NEON;
}

#elif defined(__x86_64__) || defined(_M_AMD64) // x64


namespace {
// Can be found on Intel ISA Reference for CPUID
constexpr uint32_t cpuid_avx2_bit = 1 << 5;         ///< @private Bit 5 of EBX for EAX=0x7
constexpr uint32_t cpuid_bmi1_bit = 1 << 3;         ///< @private bit 3 of EBX for EAX=0x7
constexpr uint32_t cpuid_bmi2_bit = 1 << 8;         ///< @private bit 8 of EBX for EAX=0x7
constexpr uint32_t cpuid_avx512f_bit = 1 << 16;     ///< @private bit 16 of EBX for EAX=0x7
constexpr uint32_t cpuid_avx512dq_bit = 1 << 17;    ///< @private bit 17 of EBX for EAX=0x7
constexpr uint32_t cpuid_avx512ifma_bit = 1 << 21;  ///< @private bit 21 of EBX for EAX=0x7
constexpr uint32_t cpuid_avx512pf_bit = 1 << 26;    ///< @private bit 26 of EBX for EAX=0x7
constexpr uint32_t cpuid_avx512er_bit = 1 << 27;    ///< @private bit 27 of EBX for EAX=0x7
constexpr uint32_t cpuid_avx512cd_bit = 1 << 28;    ///< @private bit 28 of EBX for EAX=0x7
constexpr uint32_t cpuid_avx512bw_bit = 1 << 30;    ///< @private bit 30 of EBX for EAX=0x7
constexpr uint32_t cpuid_avx512vl_bit = 1U << 31;    ///< @private bit 31 of EBX for EAX=0x7
constexpr uint32_t cpuid_avx512vbmi2_bit = 1 << 6;  ///< @private bit 6 of ECX for EAX=0x7
constexpr uint64_t cpuid_avx256_saved = uint64_t(1) << 2; ///< @private bit 2 = AVX
constexpr uint64_t cpuid_avx512_saved = uint64_t(7) << 5; ///< @private bits 5,6,7 = opmask, ZMM_hi256, hi16_ZMM
constexpr uint32_t cpuid_sse42_bit = 1 << 20;       ///< @private bit 20 of ECX for EAX=0x1
constexpr uint32_t cpuid_osxsave = (uint32_t(1) << 26) | (uint32_t(1) << 27); ///< @private bits 26+27 of ECX for EAX=0x1
constexpr uint32_t cpuid_pclmulqdq_bit = 1 << 1;    ///< @private bit  1 of ECX for EAX=0x1
}



static inline void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
                         uint32_t *edx) {
#if defined(_MSC_VER)
  int cpu_info[4];
  __cpuidex(cpu_info, *eax, *ecx);
  *eax = cpu_info[0];
  *ebx = cpu_info[1];
  *ecx = cpu_info[2];
  *edx = cpu_info[3];
#elif (defined(HAVE_GCC_GET_CPUID) && defined(USE_GCC_GET_CPUID)) || defined(__FILC__)
  uint32_t level = *eax;
  __get_cpuid(level, eax, ebx, ecx, edx);
#else
  uint32_t a = *eax, b, c = *ecx, d;
  asm volatile("cpuid\n\t" : "+a"(a), "=b"(b), "+c"(c), "=d"(d));
  *eax = a;
  *ebx = b;
  *ecx = c;
  *edx = d;
#endif
}


static inline uint64_t xgetbv() {
#if defined(_MSC_VER)
  return _xgetbv(0);
#elif defined(__FILC__)
  return zxgetbv();
#else
  uint32_t xcr0_lo, xcr0_hi;
  asm volatile("xgetbv\n\t" : "=a" (xcr0_lo), "=d" (xcr0_hi) : "c" (0));
  return xcr0_lo | (uint64_t(xcr0_hi) << 32);
#endif
}

static inline uint32_t detect_supported_architectures() {
  uint32_t eax, ebx, ecx, edx;
  uint32_t host_isa = 0x0;

  // EBX for EAX=0x1
  eax = 0x1;
  ecx = 0x0;
  cpuid(&eax, &ebx, &ecx, &edx);

  if (ecx & cpuid_sse42_bit) {
    host_isa |= instruction_set::SSE42;
  } else {
    return host_isa; // everything after is redundant
  }

  if (ecx & cpuid_pclmulqdq_bit) {
    host_isa |= instruction_set::PCLMULQDQ;
  }


  if ((ecx & cpuid_osxsave) != cpuid_osxsave) {
    return host_isa;
  }

  // xgetbv for checking if the OS saves registers
  uint64_t xcr0 = xgetbv();

  if ((xcr0 & cpuid_avx256_saved) == 0) {
    return host_isa;
  }

  // ECX for EAX=0x7
  eax = 0x7;
  ecx = 0x0;
  cpuid(&eax, &ebx, &ecx, &edx);
  if (ebx & cpuid_avx2_bit) {
    host_isa |= instruction_set::AVX2;
  }
  if (ebx & cpuid_bmi1_bit) {
    host_isa |= instruction_set::BMI1;
  }

  if (ebx & cpuid_bmi2_bit) {
    host_isa |= instruction_set::BMI2;
  }

  if (!((xcr0 & cpuid_avx512_saved) == cpuid_avx512_saved)) {
     return host_isa;
  }

  if (ebx & cpuid_avx512f_bit) {
    host_isa |= instruction_set::AVX512F;
  }

  if (ebx & cpuid_avx512dq_bit) {
    host_isa |= instruction_set::AVX512DQ;
  }

  if (ebx & cpuid_avx512ifma_bit) {
    host_isa |= instruction_set::AVX512IFMA;
  }

  if (ebx & cpuid_avx512pf_bit) {
    host_isa |= instruction_set::AVX512PF;
  }

  if (ebx & cpuid_avx512er_bit) {
    host_isa |= instruction_set::AVX512ER;
  }

  if (ebx & cpuid_avx512cd_bit) {
    host_isa |= instruction_set::AVX512CD;
  }

  if (ebx & cpuid_avx512bw_bit) {
    host_isa |= instruction_set::AVX512BW;
  }

  if (ebx & cpuid_avx512vl_bit) {
    host_isa |= instruction_set::AVX512VL;
  }

  if (ecx & cpuid_avx512vbmi2_bit) {
    host_isa |= instruction_set::AVX512VBMI2;
  }

  return host_isa;
}

#elif defined(__loongarch__)

static inline uint32_t detect_supported_architectures() {
  uint32_t host_isa = instruction_set::DEFAULT;
  #if defined(__linux__)
  uint64_t hwcap = 0;
  hwcap = getauxval(AT_HWCAP);
  if (hwcap & HWCAP_LOONGARCH_LSX) {
    host_isa |= instruction_set::LSX;
  }
  if (hwcap & HWCAP_LOONGARCH_LASX) {
    host_isa |= instruction_set::LASX;
  }
  #endif
  return host_isa;
}

#elif SIMDJSON_IS_RISCV64

static inline uint32_t detect_supported_architectures() {
  uint32_t host_isa = instruction_set::DEFAULT;
#if SIMDJSON_IS_RVV_VLS
  host_isa |= instruction_set::RVV_VLS;
#endif
  return host_isa;
}

#else // fallback


static inline uint32_t detect_supported_architectures() {
  return instruction_set::DEFAULT;
}


#endif // end SIMD extension detection code

} // namespace internal
} // namespace simdjson


#include <initializer_list>
#include <type_traits>

namespace simdjson {

bool implementation::supported_by_runtime_system() const {
  uint32_t required_instruction_sets = this->required_instruction_sets();
  uint32_t supported_instruction_sets = internal::detect_supported_architectures();
  return ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets);
}

} // namespace simdjson


#if SIMDJSON_IMPLEMENTATION_ARM64
#define SIMDJSON_IMPLEMENTATION arm64
#include <simdjson/generic/implementation.h>
#undef SIMDJSON_IMPLEMENTATION
namespace simdjson { namespace arm64 {
implementation::implementation() : simdjson::implementation("arm64", "ARM NEON", internal::instruction_set::NEON) {}
} }
namespace simdjson {
namespace internal {
static const arm64::implementation* get_arm64_singleton() {
  static const arm64::implementation arm64_singleton{};
  return &arm64_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_ARM64

#if SIMDJSON_IMPLEMENTATION_SVE
#define SIMDJSON_IMPLEMENTATION sve
#include <simdjson/generic/implementation.h>
#undef SIMDJSON_IMPLEMENTATION
namespace simdjson { namespace sve {
implementation::implementation() : simdjson::implementation("sve", "ARM SVE2 (std::simd, experimental)", internal::instruction_set::NEON) {}
} }
namespace simdjson {
namespace internal {
static const sve::implementation* get_sve_singleton() {
  static const sve::implementation sve_singleton{};
  return &sve_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_SVE

#if SIMDJSON_IMPLEMENTATION_FALLBACK
#define SIMDJSON_IMPLEMENTATION fallback
#include <simdjson/generic/implementation.h>
#undef SIMDJSON_IMPLEMENTATION
namespace simdjson {
namespace fallback {
implementation::implementation() : simdjson::implementation(
  "fallback", "Generic fallback implementation", 0) {}
} // namespace fallback
namespace internal {
static const fallback::implementation* get_fallback_singleton() {
  static const fallback::implementation fallback_singleton{};
  return &fallback_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_FALLBACK


#if SIMDJSON_IMPLEMENTATION_HASWELL
#define SIMDJSON_IMPLEMENTATION haswell
#include <simdjson/generic/implementation.h>
#undef SIMDJSON_IMPLEMENTATION
namespace simdjson {
namespace haswell {
// Out-of-line ctor: metadata lives here (baseline TU); the vtable/method bodies live in
// src/haswell.cpp compiled at -march=haswell.
implementation::implementation() : simdjson::implementation(
  "haswell", "Intel/AMD AVX2",
  internal::instruction_set::AVX2 | internal::instruction_set::PCLMULQDQ |
  internal::instruction_set::BMI1 | internal::instruction_set::BMI2) {}
} // namespace haswell
namespace internal {
static const haswell::implementation* get_haswell_singleton() {
  static const haswell::implementation haswell_singleton{};
  return &haswell_singleton;
}
} // namespace internal
} // namespace simdjson
#endif

#if SIMDJSON_IMPLEMENTATION_ICELAKE
#define SIMDJSON_IMPLEMENTATION icelake
#include <simdjson/generic/implementation.h>
#undef SIMDJSON_IMPLEMENTATION
namespace simdjson {
namespace icelake {
implementation::implementation() : simdjson::implementation(
  "icelake", "Intel/AMD AVX512",
  internal::instruction_set::AVX2 | internal::instruction_set::PCLMULQDQ |
  internal::instruction_set::BMI1 | internal::instruction_set::BMI2 |
  internal::instruction_set::AVX512F | internal::instruction_set::AVX512DQ |
  internal::instruction_set::AVX512CD | internal::instruction_set::AVX512BW |
  internal::instruction_set::AVX512VL | internal::instruction_set::AVX512VBMI2) {}
} // namespace icelake
namespace internal {
static const icelake::implementation* get_icelake_singleton() {
  static const icelake::implementation icelake_singleton{};
  return &icelake_singleton;
}
} // namespace internal
} // namespace simdjson
#endif

#if SIMDJSON_IMPLEMENTATION_PPC64
#define SIMDJSON_IMPLEMENTATION ppc64
#include <simdjson/generic/implementation.h>
#undef SIMDJSON_IMPLEMENTATION
namespace simdjson { namespace ppc64 {
implementation::implementation() : simdjson::implementation("ppc64", "PPC64 ALTIVEC", internal::instruction_set::ALTIVEC) {}
} }
namespace simdjson {
namespace internal {
static const ppc64::implementation* get_ppc64_singleton() {
  static const ppc64::implementation ppc64_singleton{};
  return &ppc64_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_PPC64

#if SIMDJSON_IMPLEMENTATION_WESTMERE
#define SIMDJSON_IMPLEMENTATION westmere
#include <simdjson/generic/implementation.h>
#undef SIMDJSON_IMPLEMENTATION
namespace simdjson {
namespace westmere {
implementation::implementation() : simdjson::implementation(
  "westmere", "Intel/AMD SSE4.2",
  internal::instruction_set::SSE42 | internal::instruction_set::PCLMULQDQ) {}
} // namespace westmere
namespace internal {
static const simdjson::westmere::implementation* get_westmere_singleton() {
  static const simdjson::westmere::implementation westmere_singleton{};
  return &westmere_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_WESTMERE

#if SIMDJSON_IMPLEMENTATION_LASX
#define SIMDJSON_IMPLEMENTATION lasx
#include <simdjson/generic/implementation.h>
#undef SIMDJSON_IMPLEMENTATION
namespace simdjson { namespace lasx {
implementation::implementation() : simdjson::implementation("lasx", "LoongArch ASX", internal::instruction_set::LASX) {}
} }
namespace simdjson {
namespace internal {
static const simdjson::lasx::implementation* get_lasx_singleton() {
  static const simdjson::lasx::implementation lasx_singleton{};
  return &lasx_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_LASX

#if SIMDJSON_IMPLEMENTATION_LSX
#define SIMDJSON_IMPLEMENTATION lsx
#include <simdjson/generic/implementation.h>
#undef SIMDJSON_IMPLEMENTATION
namespace simdjson { namespace lsx {
implementation::implementation() : simdjson::implementation("lsx", "LoongArch SX", internal::instruction_set::LSX) {}
} }
namespace simdjson {
namespace internal {
static const simdjson::lsx::implementation* get_lsx_singleton() {
  static const simdjson::lsx::implementation lsx_singleton{};
  return &lsx_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_LSX

#if SIMDJSON_IMPLEMENTATION_RVV_VLS
#define SIMDJSON_IMPLEMENTATION rvv_vls
#include <simdjson/generic/implementation.h>
#undef SIMDJSON_IMPLEMENTATION
namespace simdjson { namespace rvv_vls {
implementation::implementation() : simdjson::implementation("rvv_vls", "RISC-V V extension", internal::instruction_set::RVV_VLS) {}
} }
namespace simdjson {
namespace internal {
static const simdjson::rvv_vls::implementation* get_rvv_vls_singleton() {
  static const simdjson::rvv_vls::implementation rvv_vls_singleton{};
  return &rvv_vls_singleton;
}
} // namespace internal
} // namespace simdjson
#endif // SIMDJSON_IMPLEMENTATION_RVV_VLS


namespace simdjson {
namespace internal {

// When there is a single implementation, we should not pay a price
// for dispatching to the best implementation. We should just use the
// one we have. This is a compile-time check.
#define SIMDJSON_SINGLE_IMPLEMENTATION (SIMDJSON_IMPLEMENTATION_ICELAKE \
             + SIMDJSON_IMPLEMENTATION_HASWELL + SIMDJSON_IMPLEMENTATION_WESTMERE \
             + SIMDJSON_IMPLEMENTATION_ARM64 + SIMDJSON_IMPLEMENTATION_PPC64 \
             + SIMDJSON_IMPLEMENTATION_LSX + SIMDJSON_IMPLEMENTATION_LASX \
             + SIMDJSON_IMPLEMENTATION_RVV_VLS + SIMDJSON_IMPLEMENTATION_SVE \
             + SIMDJSON_IMPLEMENTATION_FALLBACK == 1)

#if SIMDJSON_SINGLE_IMPLEMENTATION
  simdjson_really_inline static const implementation* get_single_implementation() {
    return
#if SIMDJSON_IMPLEMENTATION_ICELAKE
    get_icelake_singleton();
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
    get_haswell_singleton();
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
    get_westmere_singleton();
#endif
#if SIMDJSON_IMPLEMENTATION_ARM64
    get_arm64_singleton();
#endif
#if SIMDJSON_IMPLEMENTATION_SVE
    get_sve_singleton();
#endif
#if SIMDJSON_IMPLEMENTATION_PPC64
    get_ppc64_singleton();
#endif
#if SIMDJSON_IMPLEMENTATION_LSX
    get_lsx_singleton();
#endif
#if SIMDJSON_IMPLEMENTATION_LASX
    get_lasx_singleton();
#endif
#if SIMDJSON_IMPLEMENTATION_RVV_VLS
    get_rvv_vls_singleton();
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
    get_fallback_singleton();
#endif
}
#endif

// Static array of known implementations. We're hoping these get baked into the executable
// without requiring a static initializer.

/**
 * @private Detects best supported implementation on first use, and sets it
 */
class detect_best_supported_implementation_on_first_use final : public implementation {
public:
  std::string name() const noexcept final { return set_best()->name(); }
  std::string description() const noexcept final { return set_best()->description(); }
  uint32_t required_instruction_sets() const noexcept final { return set_best()->required_instruction_sets(); }
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final {
    return set_best()->create_dom_parser_implementation(capacity, max_length, dst);
  }
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final {
    return set_best()->minify(buf, len, dst, dst_len);
  }
  simdjson_warn_unused bool validate_utf8(const char * buf, size_t len) const noexcept final override {
    return set_best()->validate_utf8(buf, len);
  }
  simdjson_inline detect_best_supported_implementation_on_first_use() noexcept : implementation("best_supported_detector", "Detects the best supported implementation and sets it", 0) {}
private:
  const implementation *set_best() const noexcept;
};

static_assert(std::is_trivially_destructible<detect_best_supported_implementation_on_first_use>::value, "detect_best_supported_implementation_on_first_use should be trivially destructible");

static const std::initializer_list<const implementation *>& get_available_implementation_pointers() {
  static const std::initializer_list<const implementation *> available_implementation_pointers {
#if SIMDJSON_IMPLEMENTATION_ICELAKE
    get_icelake_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_HASWELL
    get_haswell_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_WESTMERE
    get_westmere_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_ARM64
    get_arm64_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_SVE
    get_sve_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_PPC64
    get_ppc64_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_LASX
    get_lasx_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_LSX
    get_lsx_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_RVV_VLS
    get_rvv_vls_singleton(),
#endif
#if SIMDJSON_IMPLEMENTATION_FALLBACK
    get_fallback_singleton(),
#endif
  }; // available_implementation_pointers
  return available_implementation_pointers;
}

// So we can return UNSUPPORTED_ARCHITECTURE from the parser when there is no support
class unsupported_implementation final : public implementation {
public:
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t,
    size_t,
    std::unique_ptr<internal::dom_parser_implementation>&
  ) const noexcept final {
    return UNSUPPORTED_ARCHITECTURE;
  }
  simdjson_warn_unused error_code minify(const uint8_t *, size_t, uint8_t *, size_t &) const noexcept final override {
    return UNSUPPORTED_ARCHITECTURE;
  }
  simdjson_warn_unused bool validate_utf8(const char *, size_t) const noexcept final override {
    return false; // Just refuse to validate. Given that we have a fallback implementation
    // it seems unlikely that unsupported_implementation will ever be used. If it is used,
    // then it will flag all strings as invalid. The alternative is to return an error_code
    // from which the user has to figure out whether the string is valid UTF-8... which seems
    // like a lot of work just to handle the very unlikely case that we have an unsupported
    // implementation. And, when it does happen (that we have an unsupported implementation),
    // what are the chances that the programmer has a fallback? Given that *we* provide the
    // fallback, it implies that the programmer would need a fallback for our fallback.
  }
  unsupported_implementation() : implementation("unsupported", "Unsupported CPU (no detected SIMD instructions)", 0) {}
};

static_assert(std::is_trivially_destructible<unsupported_implementation>::value, "unsupported_singleton should be trivially destructible");

const unsupported_implementation* get_unsupported_singleton() {
    static const unsupported_implementation unsupported_singleton{};
    return &unsupported_singleton;
}

size_t available_implementation_list::size() const noexcept {
  return internal::get_available_implementation_pointers().size();
}
const implementation * const *available_implementation_list::begin() const noexcept {
  return internal::get_available_implementation_pointers().begin();
}
const implementation * const *available_implementation_list::end() const noexcept {
  return internal::get_available_implementation_pointers().end();
}
const implementation *available_implementation_list::detect_best_supported() const noexcept {
  // They are prelisted in priority order, so we just go down the list
  uint32_t supported_instruction_sets = internal::detect_supported_architectures();
  for (const implementation *impl : internal::get_available_implementation_pointers()) {
    uint32_t required_instruction_sets = impl->required_instruction_sets();
    if ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets) { return impl; }
  }
  return get_unsupported_singleton(); // this should never happen?
}

const implementation *detect_best_supported_implementation_on_first_use::set_best() const noexcept {
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  char *force_implementation_name = getenv("SIMDJSON_FORCE_IMPLEMENTATION");
  SIMDJSON_POP_DISABLE_WARNINGS

  if (force_implementation_name) {
    auto force_implementation = get_available_implementations()[force_implementation_name];
    if (force_implementation) {
      return get_active_implementation() = force_implementation;
    } else {
      // Note: abort() and stderr usage within the library is forbidden.
      return get_active_implementation() = get_unsupported_singleton();
    }
  }
  return get_active_implementation() = get_available_implementations().detect_best_supported();
}

} // namespace internal

SIMDJSON_DLLIMPORTEXPORT const internal::available_implementation_list& get_available_implementations() {
  static const internal::available_implementation_list available_implementations{};
  return available_implementations;
}

SIMDJSON_DLLIMPORTEXPORT internal::atomic_ptr<const implementation>& get_active_implementation() {
#if SIMDJSON_SINGLE_IMPLEMENTATION
  // We immediately select the only implementation we have, skipping the
  // detect_best_supported_implementation_on_first_use_singleton.
  static internal::atomic_ptr<const implementation> active_implementation{internal::get_single_implementation()};
  return active_implementation;
#else
  static const internal::detect_best_supported_implementation_on_first_use detect_best_supported_implementation_on_first_use_singleton;
  static internal::atomic_ptr<const implementation> active_implementation{&detect_best_supported_implementation_on_first_use_singleton};
  return active_implementation;
#endif
}

simdjson_warn_unused error_code minify(const char *buf, size_t len, char *dst, size_t &dst_len) noexcept {
  return get_active_implementation()->minify(reinterpret_cast<const uint8_t *>(buf), len, reinterpret_cast<uint8_t *>(dst), dst_len);
}
simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) noexcept {
  return get_active_implementation()->validate_utf8(buf, len);
}
const implementation * builtin_implementation() {
  static const implementation * builtin_impl = get_available_implementations()[SIMDJSON_STRINGIFY(SIMDJSON_BUILTIN_IMPLEMENTATION)];
  assert(builtin_impl);
  return builtin_impl;
}

} // namespace simdjson

#endif // SIMDJSON_SRC_IMPLEMENTATION_CPP
