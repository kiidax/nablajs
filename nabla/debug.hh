/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#pragma once

#ifndef NABLA_DEBUG_HH_
#define NABLA_DEBUG_HH_

#include "data.hh"
#include "context.hh"
#include "ast.hh"

#include <iostream>
#include <string>
#include <cstdio>

namespace nabla {
namespace internal {

inline std::ostream& operator << (std::ostream& os, const any_ref v) {
  if (v.is_smi()) {
    char buf[32];
#ifdef WIN32
    sprintf_s(buf, "%d", v.smi());
#else
    sprintf(buf, "%d", v.smi());
#endif
    return os << buf;
  } else {
    if (v.is_nil()) {
      return os << "(nil)";
    } else if (v.is_undefined()) {
      return os << "undefined";
    } else if (v.is_null()) {
      return os << "null";
    } else if (v.is_bool()) {
      return os << v.as<bool_data>();
    } else if (v.is_double()) {
      return os << v.as<double_data>()->data();
    } else if (v.is_u16string()) {
      u16string s = v.as<u16string_data>();
      return os << std::string(s.begin(), s.end());
    } else if (v.is<Object>()) {
      return os << "[object]";
    } else {
      return os << "[broken]";
    }
  }
}

inline std::ostream& operator << (std::ostream& os, const Position& pos) {
  os << pos.line << ":" << pos.column;
  return os;
}

inline std::ostream& operator << (std::ostream& os, const SourceLocation& loc) {
  os << "[" << loc.start << "," << loc.end << "]";
  return os;
}

}  // namespace internal
}  // namespace nabla

#endif  // NABLA_DEBUG_HH_
