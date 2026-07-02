// Generic implementation umbrella (replaces the per-ISA <isa>.h). The includer must
// `#define SIMDJSON_IMPLEMENTATION <name>` first. Self-contained: pulls the non-generic dependencies,
// the per-implementation preamble, then the DOM-side generic code, in dependency order. (No
// amalgamator machinery: each generic header carries a normal include guard and includes its own deps.)

// Non-generic dependencies (formerly generic/dependencies.h).
#include "simdjson/base.h"
#include "simdjson/implementation.h"
#include "simdjson/implementation_detection.h"
#include "simdjson/internal/instruction_set.h"
#include "simdjson/internal/dom_parser_implementation.h"
#include "simdjson/internal/jsoncharutils_tables.h"
#include "simdjson/internal/numberparsing_tables.h"
#include "simdjson/internal/simdprune_tables.h"

#include "simdjson/generic/begin.h"

// DOM-side generic code (formerly generic/amalgamated.h), in dependency order.
#include "simdjson/generic/base.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/generic/json_character_block.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/numberparsing.h"
#include "simdjson/generic/implementation_simdjson_result_base-inl.h"

// The DOM parsing ENGINE (stage1 + stage2 + the inline dom_parser_implementation bodies) is only
// instantiated where it is actually run in this TU: the fat-library backend objects (src/<isa>.cpp
// define SIMDJSON_BUILDING_IMPLEMENTATION) and header-only consumers (SIMDJSON_HEADER_ONLY). A plain
// fat-library consumer pulls only the DOM decls above and reaches the engine through the linked,
// runtime-dispatched library -- so it must NOT compile the engine (it would be unused -> -Werror).
#if defined(SIMDJSON_BUILDING_IMPLEMENTATION) || SIMDJSON_HEADER_ONLY

// Stage 1 engine (structural indexer / utf8 validator / minifier), in dependency order
// (formerly generic/stage1/amalgamated.h).
#include "simdjson/generic/stage1/base.h"
#include "simdjson/generic/stage1/buf_block_reader.h"
#include "simdjson/generic/stage1/json_escape_scanner.h"
#include "simdjson/generic/stage1/json_string_scanner.h"
#include "simdjson/generic/stage1/utf8_lookup4_algorithm.h"
#include "simdjson/generic/stage1/json_scanner.h"
#include "simdjson/generic/stage1/find_next_document_index.h"
#include "simdjson/generic/stage1/json_minifier.h"
#include "simdjson/generic/stage1/json_structural_indexer.h"
#include "simdjson/generic/stage1/utf8_validator.h"

// Stage 2 engine (DOM tape builder / string parsing), in dependency order
// (formerly generic/stage2/amalgamated.h).
#include "simdjson/generic/stage2/base.h"
#include "simdjson/generic/stage2/tape_writer.h"
#include "simdjson/generic/stage2/logger.h"
#include "simdjson/generic/stage2/json_iterator.h"
#include "simdjson/generic/stage2/stringparsing.h"
#include "simdjson/generic/stage2/structural_iterator.h"
#include "simdjson/generic/stage2/tape_builder.h"

// Header-only definitions of the vtable overrides + classify/must_be_2_3_continuation.
#include "simdjson/generic/dom_parser_implementation-inl.h"

#endif // SIMDJSON_BUILDING_IMPLEMENTATION || SIMDJSON_HEADER_ONLY

#include "simdjson/generic/end.h"
