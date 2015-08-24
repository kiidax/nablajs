/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <nabla/nabla.hh>

#include <iostream>
#include <string>
#include <cassert>

#include "data.hh"
#include "debug.hh"
#include "context.hh"

using namespace nabla::internal;

struct libtest
{
  void version_test(const std::string& test_name)
  {
    assert(nabla::major_version == 0);
    assert(nabla::minor_version == 1);
    assert(nabla::micro_version == 0);
    assert(nabla::major_version == JS_MAJOR_VERSION);
    assert(nabla::minor_version == JS_MINOR_VERSION);
    assert(nabla::micro_version == JS_MICRO_VERSION);
  }

  // Test SMI
  void type_test(const std::string& test_name)
  {
    any_ref v;
    assert(v.is_nil());

    v = 123;
    assert(!v.is_nil());
    assert(v.is_smi());
    assert(v.smi() == 123);

    v = undefined_data::alloc();
    assert(!v.is_nil());
    assert(!v.is_smi());
    assert(!v.is_double());

    v = null_data::alloc();
    assert(!v.is_nil());
    assert(!v.is_smi());
    assert(!v.is_double());

    v = bool_data::alloc(true);
    assert(!v.is_nil());
    assert(!v.is_smi());
    assert(!v.is_double());

    v = bool_data::alloc(false);
    assert(!v.is_nil());
    assert(!v.is_smi());
    assert(!v.is_double());

    v = u16string_data::alloc("abc", 3);
    assert(!v.is_nil());
    assert(!v.is_smi());
    assert(!v.is_double());

    v = double_data::alloc(-10.38);
    assert(!v.is_nil());
    assert(!v.is_smi());
    assert(v.is_double());
    assert(v.as_double() == -10.38);
  }

  void jsobj_test(const std::string& test_name)
  {
    Context* c = Context::Alloc(false);
    Object* o = Object::Alloc(nullptr);
    u16string foo("foo");
    u16string bar("bar");
    u16string baz("baz");
    any_ref v;

    v = o->Get(bar);
    assert(!v.is_nil());
    assert(v.is_undefined());

    o->Put(c, bar, foo, false);

    v = o->Get(baz);
    assert(!v.is_nil());
    assert(!v.is_nil());
    assert(v.is_undefined());

    v = o->Get(bar);
    assert(!v.is_undefined());
    assert(v.is_u16string());
    u16string ret = v.as<u16string_data>();
    std::wstring rets(ret.begin(), ret.end());
    assert(rets == L"foo");
  }
};

void run_test() {
  libtest test;
#define DO(name) test.name(#name)
  DO(version_test);
  DO(type_test);
  DO(jsobj_test);
#undef DO

#if 0
  nabla::handle v = true;
  any_ref& v2 = *reinterpret_cast<any_ref*>(&v);
  bool b = v2.as<bool_data>()->data();
  std::cerr << b << std::endl;
#endif
}

void dumpMap(any_map* m)
{
#if 0
  for (auto it = m->begin(); it != m->end(); ++it) {
    pair<map::K, map::V>& v = *it;
    std::cerr << v.first() << " -> " << v.second() << std::endl;
  }
#endif
}
