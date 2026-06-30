// simdjson C API implementation.
//
// Thin extern "C" layer over the runtime-dispatched dom parser. Compiled into libsimdjson as a
// baseline TU (no -march); all SIMD work happens behind the dom interface, which dispatches to the
// best available backend at runtime. Node handles in the C API are by-value opaque structs holding a
// trivially-copyable dom value (element/array/object/iterator = a 16-byte tape_ref); we bridge with
// memcpy, guarded by static_asserts on size and trivial-copyability.

#ifndef SIMDJSON_SRC_C_API_CPP
#define SIMDJSON_SRC_C_API_CPP

#include "simdjson/dom.h"
#include "simdjson/minify.h"
#include "simdjson/implementation.h"
#include "simdjson/simdjson_c.h"

#include <cstring>
#include <new>
#include <string_view>
#include <type_traits>

namespace {
// NOTE: do NOT `using namespace simdjson;` here -- simdjson defines a struct named
// `simdjson_error` (the exception type), which would leak into the global namespace via this
// anonymous namespace and collide with our C `simdjson_error` enum. Use explicit qualification
// and these narrow using-declarations instead.
namespace dom = simdjson::dom;
using simdjson::dom::element;
using simdjson::dom::array;
using simdjson::dom::object;

// ---- opaque-struct <-> dom value bridges (all trivially copyable, 16 bytes) ----
static_assert(std::is_trivially_copyable<element>::value, "dom::element must be trivially copyable");
static_assert(std::is_trivially_copyable<array>::value,   "dom::array must be trivially copyable");
static_assert(std::is_trivially_copyable<object>::value,  "dom::object must be trivially copyable");
static_assert(std::is_trivially_copyable<array::iterator>::value,  "array::iterator must be trivially copyable");
static_assert(std::is_trivially_copyable<object::iterator>::value, "object::iterator must be trivially copyable");
static_assert(sizeof(element) <= sizeof(simdjson_element), "simdjson_element too small");
static_assert(sizeof(array)   <= sizeof(simdjson_array),   "simdjson_array too small");
static_assert(sizeof(object)  <= sizeof(simdjson_object),  "simdjson_object too small");
static_assert(sizeof(array::iterator)  <= sizeof(simdjson_array_iter),  "simdjson_array_iter too small");
static_assert(sizeof(object::iterator) <= sizeof(simdjson_object_iter), "simdjson_object_iter too small");

// error/type enum parity (same order from 0 / same char codes)
static_assert((int)SIMDJSON_C_NUM_ERROR_CODES == (int)simdjson::NUM_ERROR_CODES, "error_code list drifted");
static_assert((int)SIMDJSON_C_INCORRECT_TYPE == (int)simdjson::INCORRECT_TYPE, "error_code mapping drifted");
static_assert((int)SIMDJSON_C_NO_SUCH_FIELD == (int)simdjson::NO_SUCH_FIELD, "error_code mapping drifted");
static_assert((int)SIMDJSON_C_TYPE_OBJECT == (int)simdjson::dom::element_type::OBJECT, "element_type drifted");
static_assert((int)SIMDJSON_C_TYPE_STRING == (int)simdjson::dom::element_type::STRING, "element_type drifted");

template <typename Dom, typename C> static inline C pack(const Dom &v) {
  C c; std::memcpy(&c, &v, sizeof(Dom)); return c;
}
template <typename Dom, typename C> static inline Dom unpack(const C &c) {
  Dom v{}; std::memcpy(&v, &c, sizeof(Dom)); return v;
}

static inline simdjson_element  e2c(const element &e) { return pack<element, simdjson_element>(e); }
static inline element           c2e(const simdjson_element &c) { return unpack<element, simdjson_element>(c); }
static inline simdjson_array    a2c(const array &a) { return pack<array, simdjson_array>(a); }
static inline array             c2a(const simdjson_array &c) { return unpack<array, simdjson_array>(c); }
static inline simdjson_object   o2c(const object &o) { return pack<object, simdjson_object>(o); }
static inline object            c2o(const simdjson_object &c) { return unpack<object, simdjson_object>(c); }
static inline simdjson_array_iter  ai2c(const array::iterator &i) { return pack<array::iterator, simdjson_array_iter>(i); }
static inline array::iterator      c2ai(const simdjson_array_iter &c) { return unpack<array::iterator, simdjson_array_iter>(c); }
static inline simdjson_object_iter oi2c(const object::iterator &i) { return pack<object::iterator, simdjson_object_iter>(i); }
static inline object::iterator     c2oi(const simdjson_object_iter &c) { return unpack<object::iterator, simdjson_object_iter>(c); }

} // anonymous namespace

struct simdjson_parser { simdjson::dom::parser impl; };

extern "C" {

simdjson_parser *simdjson_parser_new(void) {
  return new (std::nothrow) simdjson_parser();
}
simdjson_parser *simdjson_parser_new_capacity(size_t max_capacity) {
  return new (std::nothrow) simdjson_parser{ simdjson::dom::parser(max_capacity) };
}
void simdjson_parser_free(simdjson_parser *parser) { delete parser; }

simdjson_error simdjson_parse(simdjson_parser *parser, const char *buf, size_t len,
                              simdjson_element *out) {
  if (parser == nullptr) { return SIMDJSON_C_UNINITIALIZED; }
  element e;
  auto err = parser->impl.parse(buf, len, /*realloc_if_needed*/ true).get(e);
  if (!err) { *out = e2c(e); }
  return (simdjson_error)err;
}

const char *simdjson_error_message(simdjson_error error) {
  return simdjson::error_message((simdjson::error_code)error);
}
simdjson_error simdjson_minify(const char *buf, size_t len, char *dst, size_t *dst_len) {
  size_t written = 0;
  auto err = simdjson::minify(buf, len, dst, written);
  *dst_len = written;
  return (simdjson_error)err;
}
int simdjson_validate_utf8(const char *buf, size_t len) {
  return simdjson::validate_utf8(buf, len) ? 1 : 0;
}

simdjson_type simdjson_element_type(simdjson_element element) {
  return (simdjson_type)c2e(element).type();
}
int simdjson_element_is_null(simdjson_element element) {
  return c2e(element).is_null() ? 1 : 0;
}
simdjson_error simdjson_element_get_int64(simdjson_element element, int64_t *out) {
  return (simdjson_error)c2e(element).get_int64().get(*out);
}
simdjson_error simdjson_element_get_uint64(simdjson_element element, uint64_t *out) {
  return (simdjson_error)c2e(element).get_uint64().get(*out);
}
simdjson_error simdjson_element_get_double(simdjson_element element, double *out) {
  return (simdjson_error)c2e(element).get_double().get(*out);
}
simdjson_error simdjson_element_get_bool(simdjson_element element, int *out) {
  bool b = false;
  auto err = c2e(element).get_bool().get(b);
  if (!err) { *out = b ? 1 : 0; }
  return (simdjson_error)err;
}
simdjson_error simdjson_element_get_string(simdjson_element element, const char **out_ptr,
                                           size_t *out_len) {
  std::string_view sv;
  auto err = c2e(element).get_string().get(sv);
  if (!err) { *out_ptr = sv.data(); *out_len = sv.size(); }
  return (simdjson_error)err;
}

simdjson_error simdjson_element_get_array(simdjson_element element, simdjson_array *out) {
  array a;
  auto err = c2e(element).get_array().get(a);
  if (!err) { *out = a2c(a); }
  return (simdjson_error)err;
}
simdjson_error simdjson_element_get_object(simdjson_element element, simdjson_object *out) {
  object o;
  auto err = c2e(element).get_object().get(o);
  if (!err) { *out = o2c(o); }
  return (simdjson_error)err;
}
simdjson_error simdjson_element_at_key(simdjson_element element, const char *key, size_t key_len,
                                       simdjson_element *out) {
  dom::element r;
  auto err = c2e(element).at_key(std::string_view(key, key_len)).get(r);
  if (!err) { *out = e2c(r); }
  return (simdjson_error)err;
}
simdjson_error simdjson_element_at_index(simdjson_element element, size_t index,
                                         simdjson_element *out) {
  dom::element r;
  auto err = c2e(element).at(index).get(r);
  if (!err) { *out = e2c(r); }
  return (simdjson_error)err;
}
simdjson_error simdjson_element_at_pointer(simdjson_element element, const char *json_pointer,
                                           size_t len, simdjson_element *out) {
  dom::element r;
  auto err = c2e(element).at_pointer(std::string_view(json_pointer, len)).get(r);
  if (!err) { *out = e2c(r); }
  return (simdjson_error)err;
}
simdjson_error simdjson_element_at_path(simdjson_element element, const char *json_path,
                                        size_t len, simdjson_element *out) {
  dom::element r;
  auto err = c2e(element).at_path(std::string_view(json_path, len)).get(r);
  if (!err) { *out = e2c(r); }
  return (simdjson_error)err;
}

size_t simdjson_array_size(simdjson_array array) { return c2a(array).size(); }
simdjson_error simdjson_array_at(simdjson_array array, size_t index, simdjson_element *out) {
  dom::element r;
  auto err = c2a(array).at(index).get(r);
  if (!err) { *out = e2c(r); }
  return (simdjson_error)err;
}
simdjson_array_iter simdjson_array_iter_begin(simdjson_array array) { return ai2c(c2a(array).begin()); }
simdjson_array_iter simdjson_array_iter_end(simdjson_array array) { return ai2c(c2a(array).end()); }
int simdjson_array_iter_equal(simdjson_array_iter a, simdjson_array_iter b) {
  return c2ai(a) == c2ai(b) ? 1 : 0;
}
simdjson_element simdjson_array_iter_get(simdjson_array_iter it) { return e2c(*c2ai(it)); }
void simdjson_array_iter_advance(simdjson_array_iter *it) {
  auto i = c2ai(*it); ++i; *it = ai2c(i);
}

size_t simdjson_object_size(simdjson_object object) { return c2o(object).size(); }
simdjson_error simdjson_object_get(simdjson_object object, const char *key, size_t key_len,
                                   simdjson_element *out) {
  dom::element r;
  auto err = c2o(object).at_key(std::string_view(key, key_len)).get(r);
  if (!err) { *out = e2c(r); }
  return (simdjson_error)err;
}
simdjson_error simdjson_object_get_case_insensitive(simdjson_object object, const char *key,
                                                    size_t key_len, simdjson_element *out) {
  dom::element r;
  auto err = c2o(object).at_key_case_insensitive(std::string_view(key, key_len)).get(r);
  if (!err) { *out = e2c(r); }
  return (simdjson_error)err;
}
simdjson_object_iter simdjson_object_iter_begin(simdjson_object object) { return oi2c(c2o(object).begin()); }
simdjson_object_iter simdjson_object_iter_end(simdjson_object object) { return oi2c(c2o(object).end()); }
int simdjson_object_iter_equal(simdjson_object_iter a, simdjson_object_iter b) {
  return c2oi(a) == c2oi(b) ? 1 : 0;
}
simdjson_element simdjson_object_iter_value(simdjson_object_iter it) { return e2c(c2oi(it).value()); }
void simdjson_object_iter_key(simdjson_object_iter it, const char **key, size_t *key_len) {
  std::string_view sv = c2oi(it).key();
  *key = sv.data(); *key_len = sv.size();
}
void simdjson_object_iter_advance(simdjson_object_iter *it) {
  auto i = c2oi(*it); ++i; *it = oi2c(i);
}

} // extern "C"

#endif // SIMDJSON_SRC_C_API_CPP
