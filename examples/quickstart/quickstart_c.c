/*
 * Quick-start: use simdjson from C via the extern "C" API.
 *
 * This is the drop-in path for consumers that cannot (or do not want to) compile the C++26
 * headers: include <simdjson/simdjson_c.h> and link the prebuilt, runtime-dispatched library.
 * Compiles as plain C; the library detects the CPU and dispatches to the best backend at runtime.
 */
#include <simdjson/simdjson_c.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  const char *json = "{\"search_metadata\":{\"count\":100}}";

  simdjson_parser *parser = simdjson_parser_new();
  if (!parser) { fprintf(stderr, "allocation failed\n"); return 1; }

  simdjson_element root, meta, count;
  simdjson_error err = simdjson_parse(parser, json, strlen(json), &root);
  if (!err) err = simdjson_element_at_key(root, "search_metadata", strlen("search_metadata"), &meta);
  if (!err) err = simdjson_element_at_key(meta, "count", strlen("count"), &count);

  int64_t n = 0;
  if (!err) err = simdjson_element_get_int64(count, &n);
  if (err) {
    fprintf(stderr, "simdjson error: %s\n", simdjson_error_message(err));
    simdjson_parser_free(parser);
    return 1;
  }

  printf("%lld results.\n", (long long)n);
  simdjson_parser_free(parser);
  return n == 100 ? 0 : 1;
}
