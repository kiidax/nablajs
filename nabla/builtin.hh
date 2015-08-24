/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#pragma once

#ifndef NABLA_BUILTIN_HH_
#define NABLA_BUILTIN_HH_

#include <pcre.h>

#include "coll.hh"
#include "context.hh"
#include "data.hh"

namespace nabla {
namespace internal {

class RegExp : public heap_data {
 public:
  enum {
    kNone = 0x00,
    kGlobal = 0x01,
    kIgnoreCase = 0x02,
    kMultiline = 0x04
  };

 public:
  static const tag class_tag = kTagRegExp;
  static RegExp* Alloc();

 public:
  pcre16 *re;
  int flags;
};

}  // namespace internal
}  // namespace nabla

#endif  // NABLA_BUILTIN_HH_
