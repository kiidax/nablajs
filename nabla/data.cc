/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "data.hh"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <gc/gc.h>

#include "debug.hh"

namespace nabla {
namespace internal {

void* gc_malloc(size_t n) {
  return GC_MALLOC(n + 100);
}

void* gc_realloc(void* p, size_t n) {
  return GC_REALLOC(p, n + 100);
}

void* gc_malloc_atomic(size_t n) {
  return GC_MALLOC_ATOMIC(n + 100);
}

void init() {
  GC_INIT();
  heap_data::value_table_[heap_data::undefined_index_].tag_ = heap_data::kTagUndefined;
  heap_data::value_table_[heap_data::nullvalue_index_].tag_ = heap_data::kTagNull;
  heap_data::value_table_[heap_data::true_index_].tag_ = heap_data::kTagBool;
  heap_data::value_table_[heap_data::false_index_].tag_ = heap_data::kTagBool;
}

void getmeminfo(size_t& heap_size, size_t& free_bytes) {
  heap_size = GC_get_heap_size();
  free_bytes = GC_get_free_bytes();
}

void gc() {
  GC_gcollect();
}

template<>
string<char16_t>::string(const char* s) : string(s, strlen(s)) {
}

any_ref::any_ref(bool b) {
  data_ = reinterpret_cast<intptr_t>(bool_data::alloc(b));
}

any_ref::any_ref(const char* s) : any_ref(s, strlen(s)) {
}

heap_data heap_data::value_table_[];

undefined_data* undefined_data::alloc()
{
  return reinterpret_cast<undefined_data*>(&heap_data::value_table_[heap_data::undefined_index_]);
}

null_data* null_data::alloc() {
  return reinterpret_cast<null_data*>(&heap_data::value_table_[heap_data::nullvalue_index_]);
}

bool_data* bool_data::alloc(bool b) {
  if (b) {
    return reinterpret_cast<bool_data*>(&heap_data::value_table_[heap_data::true_index_]);
  } else {
    return reinterpret_cast<bool_data*>(&heap_data::value_table_[heap_data::false_index_]);
  }
}

double_data* double_data::alloc(double d) {
  double_data* ret = reinterpret_cast<double_data*>(GC_MALLOC_ATOMIC(sizeof (double_data)));
  if (!ret) return nullptr;
  ret->tag_ = kTagDouble;
  ret->data_ = d;
  return ret;
}

string_data_base* string_data_base::alloc_(size_t n, size_t charsize) {
  size_t size = sizeof(string_data_base) + n * charsize;
  string_data_base* ret = reinterpret_cast<string_data_base*>(GC_MALLOC_ATOMIC(size));
  if (!ret) return nullptr;
  ret->length_ = n;
  return ret;
}

}  // namespace internal
}  // namespace nabla
