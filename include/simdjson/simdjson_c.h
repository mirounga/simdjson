/*
 * simdjson C API
 * --------------
 * A C-linkage (extern "C") wrapper over simdjson's runtime-dispatched DOM parser.
 * The compiled library detects the CPU at first use and dispatches to the most capable
 * backend; this header exposes that to C / FFI clients. It is valid C99 (no C++ types).
 *
 * Memory & lifetime:
 *  - A `simdjson_parser` owns the document produced by the last successful `simdjson_parse`.
 *    All node handles (element/array/object/iterators) point INTO that document and remain
 *    valid only until the next `simdjson_parse` on the same parser or `simdjson_parser_free`.
 *  - Node handles are small by-value structs (no allocation); copy them freely.
 *  - A parser is NOT thread-safe and cannot parse two documents at once.
 *
 * Error handling: functions return `simdjson_error` (SIMDJSON_C_SUCCESS == 0); results are
 * delivered through out-parameters. Use `simdjson_error_message` for a human-readable string.
 */
#ifndef SIMDJSON_C_H
#define SIMDJSON_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes. Values match simdjson::error_code (same order from 0). */
typedef enum {
  SIMDJSON_C_SUCCESS = 0,
  SIMDJSON_C_CAPACITY,
  SIMDJSON_C_MEMALLOC,
  SIMDJSON_C_TAPE_ERROR,
  SIMDJSON_C_DEPTH_ERROR,
  SIMDJSON_C_STRING_ERROR,
  SIMDJSON_C_T_ATOM_ERROR,
  SIMDJSON_C_F_ATOM_ERROR,
  SIMDJSON_C_N_ATOM_ERROR,
  SIMDJSON_C_NUMBER_ERROR,
  SIMDJSON_C_BIGINT_ERROR,
  SIMDJSON_C_UTF8_ERROR,
  SIMDJSON_C_UNINITIALIZED,
  SIMDJSON_C_EMPTY,
  SIMDJSON_C_UNESCAPED_CHARS,
  SIMDJSON_C_UNCLOSED_STRING,
  SIMDJSON_C_UNSUPPORTED_ARCHITECTURE,
  SIMDJSON_C_INCORRECT_TYPE,
  SIMDJSON_C_NUMBER_OUT_OF_RANGE,
  SIMDJSON_C_INDEX_OUT_OF_BOUNDS,
  SIMDJSON_C_NO_SUCH_FIELD,
  SIMDJSON_C_IO_ERROR,
  SIMDJSON_C_INVALID_JSON_POINTER,
  SIMDJSON_C_INVALID_URI_FRAGMENT,
  SIMDJSON_C_UNEXPECTED_ERROR,
  SIMDJSON_C_PARSER_IN_USE,
  SIMDJSON_C_OUT_OF_ORDER_ITERATION,
  SIMDJSON_C_INSUFFICIENT_PADDING,
  SIMDJSON_C_INCOMPLETE_ARRAY_OR_OBJECT,
  SIMDJSON_C_SCALAR_DOCUMENT_AS_VALUE,
  SIMDJSON_C_OUT_OF_BOUNDS,
  SIMDJSON_C_TRAILING_CONTENT,
  SIMDJSON_C_OUT_OF_CAPACITY,
  SIMDJSON_C_NUM_ERROR_CODES
} simdjson_error;

/* Element types. Values match simdjson::dom::element_type. */
typedef enum {
  SIMDJSON_C_TYPE_ARRAY  = '[',
  SIMDJSON_C_TYPE_OBJECT = '{',
  SIMDJSON_C_TYPE_INT64  = 'l',
  SIMDJSON_C_TYPE_UINT64 = 'u',
  SIMDJSON_C_TYPE_DOUBLE = 'd',
  SIMDJSON_C_TYPE_STRING = '"',
  SIMDJSON_C_TYPE_BOOL   = 't',
  SIMDJSON_C_TYPE_NULL   = 'n',
  SIMDJSON_C_TYPE_BIGINT = 'Z'
} simdjson_type;

/* Opaque, heap-owned parser. */
typedef struct simdjson_parser simdjson_parser;

/* By-value node handles: opaque fixed-size buffers holding a trivially-copyable dom value.
 * 16 bytes covers dom::element/array/object (an internal tape_ref). Iterators get 32 bytes
 * of headroom. The implementation static_asserts these sizes against the real C++ types. */
typedef struct { uint64_t _opaque[2]; } simdjson_element;
typedef struct { uint64_t _opaque[2]; } simdjson_array;
typedef struct { uint64_t _opaque[2]; } simdjson_object;
typedef struct { uint64_t _opaque[4]; } simdjson_array_iter;
typedef struct { uint64_t _opaque[4]; } simdjson_object_iter;

/* ---- lifecycle ---- */
simdjson_parser *simdjson_parser_new(void);
simdjson_parser *simdjson_parser_new_capacity(size_t max_capacity);
void             simdjson_parser_free(simdjson_parser *parser);

/* Parse `len` bytes at `buf` (raw, unpadded input is fine; the parser copies/pads as needed).
 * On success writes the document root to *out. The result is owned by `parser`. */
simdjson_error simdjson_parse(simdjson_parser *parser, const char *buf, size_t len,
                              simdjson_element *out);

/* ---- dispatched free functions (no parser needed) ---- */
const char    *simdjson_error_message(simdjson_error error);
/* `dst` must have room for at least `len` bytes; *dst_len gets the number written. */
simdjson_error simdjson_minify(const char *buf, size_t len, char *dst, size_t *dst_len);
/* Returns 1 if valid UTF-8, 0 otherwise. */
int            simdjson_validate_utf8(const char *buf, size_t len);

/* ---- element: type + scalars ---- */
simdjson_type  simdjson_element_type(simdjson_element element);
int            simdjson_element_is_null(simdjson_element element);
simdjson_error simdjson_element_get_int64(simdjson_element element, int64_t *out);
simdjson_error simdjson_element_get_uint64(simdjson_element element, uint64_t *out);
simdjson_error simdjson_element_get_double(simdjson_element element, double *out);
simdjson_error simdjson_element_get_bool(simdjson_element element, int *out);
/* On success *out_ptr points into the parser's buffer (valid while the parser/document live);
 * *out_len is the byte length. Not NUL-terminated. */
simdjson_error simdjson_element_get_string(simdjson_element element, const char **out_ptr,
                                           size_t *out_len);

/* ---- element: containers + navigation ---- */
simdjson_error simdjson_element_get_array(simdjson_element element, simdjson_array *out);
simdjson_error simdjson_element_get_object(simdjson_element element, simdjson_object *out);
simdjson_error simdjson_element_at_key(simdjson_element element, const char *key, size_t key_len,
                                       simdjson_element *out);
simdjson_error simdjson_element_at_index(simdjson_element element, size_t index,
                                         simdjson_element *out);
simdjson_error simdjson_element_at_pointer(simdjson_element element, const char *json_pointer,
                                           size_t len, simdjson_element *out);
simdjson_error simdjson_element_at_path(simdjson_element element, const char *json_path,
                                        size_t len, simdjson_element *out);

/* ---- array ---- */
size_t         simdjson_array_size(simdjson_array array);
simdjson_error simdjson_array_at(simdjson_array array, size_t index, simdjson_element *out);
simdjson_array_iter simdjson_array_iter_begin(simdjson_array array);
simdjson_array_iter simdjson_array_iter_end(simdjson_array array);
int            simdjson_array_iter_equal(simdjson_array_iter a, simdjson_array_iter b);
simdjson_element simdjson_array_iter_get(simdjson_array_iter it);
void           simdjson_array_iter_advance(simdjson_array_iter *it);

/* ---- object ---- */
size_t         simdjson_object_size(simdjson_object object);
simdjson_error simdjson_object_get(simdjson_object object, const char *key, size_t key_len,
                                   simdjson_element *out);
simdjson_error simdjson_object_get_case_insensitive(simdjson_object object, const char *key,
                                                    size_t key_len, simdjson_element *out);
simdjson_object_iter simdjson_object_iter_begin(simdjson_object object);
simdjson_object_iter simdjson_object_iter_end(simdjson_object object);
int            simdjson_object_iter_equal(simdjson_object_iter a, simdjson_object_iter b);
simdjson_element simdjson_object_iter_value(simdjson_object_iter it);
/* *key points into the parser buffer; *key_len is its length. */
void           simdjson_object_iter_key(simdjson_object_iter it, const char **key, size_t *key_len);
void           simdjson_object_iter_advance(simdjson_object_iter *it);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SIMDJSON_C_H */
