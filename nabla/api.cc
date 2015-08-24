/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#include <nabla/nabla.hh>
// config.h is after nabla/nabla.hh so that we make sure that it
// nabla/nabla.hh won't depend on config.h
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gc/gc.h>
#include "data.hh"
#include "context.hh"

namespace nabla {

const int major_version = JS_MAJOR_VERSION;
const int minor_version = JS_MINOR_VERSION;
const int micro_version = JS_MICRO_VERSION;

void init() {
  nabla::internal::init();
}

void gc() {
  nabla::internal::gc();
}

void getmeminfo(meminfo* info) {
  nabla::internal::getmeminfo(info->heap_size, info->free_bytes);
}

context::context() {
  nabla::internal::Context** data = reinterpret_cast<nabla::internal::Context**>(GC_MALLOC_UNCOLLECTABLE(sizeof (nabla::internal::Context*)));
  *data = nabla::internal::Context::Alloc(true);
  data_ = data;
}

context::~context() {
  GC_FREE(data_);
}

bool context::eval(const std::u16string& source, const std::u16string& name, std::u16string& r) {
  nabla::internal::Thread th;
  nabla::internal::Context** data = reinterpret_cast<nabla::internal::Context**>(data_);
  nabla::internal::Context* c = *data;
  nabla::internal::u16string _source(source.data(), source.length());
  nabla::internal::u16string _name(name.data(), name.length());
  nabla::internal::any_ref val = c->EvalString(_source, _name);
  if (!val) {
    val = nabla::internal::Catch();
  }
  assert(!!val);
  if (val.is_undefined())
    return false;
  nabla::internal::u16string result = ToString(c, val);
  if (!result) return false;
  r = std::u16string(result.begin(), result.end());
  return true;
}

}  // namespace nabla
