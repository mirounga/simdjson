// Generic per-implementation epilogue. Deliberately does NOT #undef SIMDJSON_IMPLEMENTATION:
// with amalgamation removed there is exactly one implementation per translation unit, so the macro
// (set once by the includer) stays valid for the whole TU, including the second begin/body region
// in src/<isa>.cpp. (implementation.cpp, which declares several variants in one TU, manages its own
// #define/#undef around each include.)
