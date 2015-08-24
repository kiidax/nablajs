/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#pragma once

#ifndef NABLA_HH_
#define NABLA_HH_

#include <cstdint>
#include <string>

#define JS_MAJOR_VERSION 0
#define JS_MINOR_VERSION 1
#define JS_MICRO_VERSION 1

namespace nabla {

extern const int major_version;
extern const int minor_version;
extern const int micro_version;

struct meminfo;

void init();
void gc();
void getmeminfo(meminfo* info);

class context {
 public:
  context();
  ~context();
  bool eval(const std::u16string& source, const std::u16string& name, std::u16string& r);

 private:
  void* data_;
};

struct meminfo {
  size_t heap_size;
  size_t free_bytes;
};

}  // namespace nabla

#endif  // NABLA_HH_
