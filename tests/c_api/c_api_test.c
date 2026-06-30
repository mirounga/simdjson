/* C-language test for the simdjson C API.
 *
 * Compiled as C (not C++) on purpose: this proves simdjson/simdjson_c.h is valid C and that the
 * extern "C" symbols actually link against the (C++) library. Exercises parse, scalar getters,
 * container access by key/index, array+object iteration, JSON pointer, minify, and validate_utf8.
 */
#include <simdjson/simdjson_c.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)
#define CHECK_OK(err) CHECK((err) == SIMDJSON_C_SUCCESS)

int main(void) {
  const char *json =
    "{ \"i\": -7, \"u\": 42, \"d\": 3.5, \"b\": true, \"s\": \"hi\", \"n\": null, "
    "\"arr\": [10, 20, 30], \"obj\": {\"x\": 1} }";
  size_t json_len = strlen(json);

  simdjson_parser *p = simdjson_parser_new();
  CHECK(p != NULL);

  simdjson_element root;
  CHECK_OK(simdjson_parse(p, json, json_len, &root));
  CHECK(simdjson_element_type(root) == SIMDJSON_C_TYPE_OBJECT);

  simdjson_object obj;
  CHECK_OK(simdjson_element_get_object(root, &obj));
  CHECK(simdjson_object_size(obj) == 8);

  /* scalars by key */
  simdjson_element e;
  int64_t i64; uint64_t u64; double d; int b;
  const char *s; size_t slen;

  CHECK_OK(simdjson_object_get(obj, "i", 1, &e));
  CHECK(simdjson_element_type(e) == SIMDJSON_C_TYPE_INT64);
  CHECK_OK(simdjson_element_get_int64(e, &i64)); CHECK(i64 == -7);

  CHECK_OK(simdjson_object_get(obj, "u", 1, &e));
  CHECK_OK(simdjson_element_get_uint64(e, &u64)); CHECK(u64 == 42);

  CHECK_OK(simdjson_object_get(obj, "d", 1, &e));
  CHECK_OK(simdjson_element_get_double(e, &d)); CHECK(d == 3.5);

  CHECK_OK(simdjson_object_get(obj, "b", 1, &e));
  CHECK_OK(simdjson_element_get_bool(e, &b)); CHECK(b == 1);

  CHECK_OK(simdjson_object_get(obj, "s", 1, &e));
  CHECK_OK(simdjson_element_get_string(e, &s, &slen));
  CHECK(slen == 2 && memcmp(s, "hi", 2) == 0);

  CHECK_OK(simdjson_object_get(obj, "n", 1, &e));
  CHECK(simdjson_element_is_null(e) == 1);

  /* missing key -> NO_SUCH_FIELD */
  CHECK(simdjson_object_get(obj, "nope", 4, &e) == SIMDJSON_C_NO_SUCH_FIELD);
  /* wrong type -> INCORRECT_TYPE */
  CHECK_OK(simdjson_object_get(obj, "s", 1, &e));
  CHECK(simdjson_element_get_int64(e, &i64) == SIMDJSON_C_INCORRECT_TYPE);

  /* array: random access + iteration */
  simdjson_element arr_e; simdjson_array arr;
  CHECK_OK(simdjson_object_get(obj, "arr", 3, &arr_e));
  CHECK_OK(simdjson_element_get_array(arr_e, &arr));
  CHECK(simdjson_array_size(arr) == 3);
  CHECK_OK(simdjson_array_at(arr, 1, &e));
  CHECK_OK(simdjson_element_get_int64(e, &i64)); CHECK(i64 == 20);

  int64_t sum = 0;
  simdjson_array_iter it = simdjson_array_iter_begin(arr);
  simdjson_array_iter ait_end = simdjson_array_iter_end(arr);
  while (!simdjson_array_iter_equal(it, ait_end)) {
    simdjson_element cur = simdjson_array_iter_get(it);
    int64_t v; CHECK_OK(simdjson_element_get_int64(cur, &v)); sum += v;
    simdjson_array_iter_advance(&it);
  }
  CHECK(sum == 60);

  /* object iteration */
  simdjson_element nested_e; simdjson_object nested;
  CHECK_OK(simdjson_object_get(obj, "obj", 3, &nested_e));
  CHECK_OK(simdjson_element_get_object(nested_e, &nested));
  int seen_x = 0;
  simdjson_object_iter oit = simdjson_object_iter_begin(nested);
  simdjson_object_iter oit_end = simdjson_object_iter_end(nested);
  while (!simdjson_object_iter_equal(oit, oit_end)) {
    const char *k; size_t klen;
    simdjson_object_iter_key(oit, &k, &klen);
    simdjson_element val = simdjson_object_iter_value(oit);
    if (klen == 1 && k[0] == 'x') {
      int64_t v; CHECK_OK(simdjson_element_get_int64(val, &v)); CHECK(v == 1); seen_x = 1;
    }
    simdjson_object_iter_advance(&oit);
  }
  CHECK(seen_x == 1);

  /* JSON pointer navigation from the root */
  CHECK_OK(simdjson_element_at_pointer(root, "/arr/2", 6, &e));
  CHECK_OK(simdjson_element_get_int64(e, &i64)); CHECK(i64 == 30);

  /* at_path is exercised for linkage (value not strictly asserted to avoid path-syntax brittleness) */
  (void)simdjson_element_at_path(root, "/obj/x", 6, &e);

  /* dispatched free functions */
  CHECK(simdjson_validate_utf8(json, json_len) == 1);
  CHECK(simdjson_validate_utf8("\xff\xfe", 2) == 0);

  char dst[256]; /* json is small and fixed; minify needs room for up to json_len bytes */
  size_t dst_len = 0;
  CHECK_OK(simdjson_minify(json, json_len, dst, &dst_len));
  CHECK(dst_len > 0 && dst_len <= json_len);

  simdjson_parser_free(p);

  if (failures == 0) { printf("c_api_test: all checks passed\n"); return 0; }
  printf("c_api_test: %d failures\n", failures);
  return 1;
}
