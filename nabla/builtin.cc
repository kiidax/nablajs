/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "builtin.hh"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <gc/gc.h>
#include <pcre.h>
#include <string>

#include "context.hh"
#include "debug.hh"

namespace nabla {
namespace internal {

#define GET_ARG(n) (argc > (n) ? argv[n] : undefined_data::alloc())

struct func_spec {
  const char* name;
  NativeCodeProc native_code;
};

// 15.1 The Global Object

static any_ref Global_eval(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  if (argc < 2) return undefined_data::alloc();
  any_ref v = argv[1];
  if (!v.is_u16string()) return v;
  u16string s = v.as_u16string() + ";";
  any_ref rval = c->EvalString(s, "");
  return rval;
}

static any_ref Global_parseFloat(Context* c, size_t argc, const any_ref* argv) {
  // 15.1.2.3 parseFloat (string)
  if (!argv[0]) return ThrowTypeError(c);
  if (argc < 2) return 0;
  u16string s = ToString(c, argv[1]);
  if (!s) return nullptr;
  std::string u8s(s.begin(), s.end());
  double dval = strtod(u8s.c_str(), nullptr);
  return dval;
}

// Extension global functions

static any_ref Global_evalcx(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);

  u16string s;
  if (argc >= 2) {
    s = ToString(c, argv[1]);
    if (!s) return nullptr;
  }

  Object* o = nullptr;
  if (argc >= 3) {
    any_ref v = argv[2];
    if (!(v.is_undefined() || v.is_null())) {
      o = ToObject(c, v);
      if (!o) return nullptr;
    }
  }

  Context* eval_ctx;
  if (!o) {
    eval_ctx = Context::Alloc(false);
    o = eval_ctx->global_obj();
  } else {
    if (!o->host_data.is<Context>())
      return ThrowTypeError(c);
    eval_ctx = o->host_data.as<Context>();
  }

  if (s.length() == 0) {
    return o;
  } else {
    s = s + ";";
    return eval_ctx->EvalString(s, "");
  }
}

static any_ref Global_load(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  any_ref rval = undefined_data::alloc();
  for (const any_ref* it = &argv[1]; it != &argv[argc]; ++it) {
    any_ref fn_val = *it;
    u16string fn_str = ToString(c, fn_val);
    if (!fn_str) return nullptr;
    std::string path(fn_str.begin(), fn_str.end());
    std::fstream fin(path);
    if (!fin) return ThrowTypeError(c);
    std::u16string d((std::istreambuf_iterator<char>(fin)),
                     (std::istreambuf_iterator<char>()));
    fin.close();
    u16string source_str(d.data(), d.length());
    rval = c->EvalString(source_str, fn_str);
    if (!rval) return nullptr;
  }
  return rval;
}

static any_ref Global_read(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  u16string filename_str;
  if (argc < 2) {
    filename_str = "undefined";
  } else {
    filename_str = ToString(c, argv[1]);
    if (!filename_str) return nullptr;
  }
  std::string path(filename_str.begin(), filename_str.end());
  std::fstream fin(path);
  if (!fin) {
    Object* o = Object::Alloc(c->error_proto());
    o->Put(c, "message", "File error", false);
    Throw(o);
    return nullptr;
  }
  std::u16string d((std::istreambuf_iterator<char>(fin)),
                   (std::istreambuf_iterator<char>()));
  fin.close();
  return u16string(d.data(), d.length());
}

static any_ref Global_print(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  for (size_t i = 1; i < argc; i++) {
    u16string s = ToString(c, argv[i]);
    if (!s) return nullptr;
    if (i != 1) std::cout << ' ';
    std::cout << std::string(s.begin(), s.end());
  }
  std::cout << std::endl;
  return undefined_data::alloc();
}

static any_ref Glolbal_quit(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  int code = EXIT_SUCCESS;
  if (argc >= 2) {
    if (!ToInteger<int>(c, argv[1], code)) return nullptr;
  }
  std::exit(code);
  return undefined_data::alloc();
}

// 15.2 Object Objects

static any_ref Object_construct(Context* c, size_t argc, const any_ref* argv) {
  Object* this_obj;
  if (!argv[0])
    this_obj = Object::Alloc(c->object_proto());
  else
    this_obj = argv[0].as<Object>();
  return this_obj;
}

static any_ref Object_getPrototypeOf(Context* c, size_t argc, const any_ref* argv) {
  // 15.2.3.2 Object.getPrototypeOf ( O )
  if (!argv[0]) return ThrowTypeError(c);
  any_ref v = GET_ARG(1);
  if (!v.is<Object>()) return ThrowTypeError(c);
  Object* o = v.as<Object>();
  Object* proto = o->proto();
  if (!proto) return null_data::alloc();
  return proto;
}

static any_ref Object_getOwnPropertyDescriptor(Context* c, size_t argc, const any_ref* argv) {
  // 15.2.3.3 Object.getOwnPropertyDescriptor ( O, P )
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  any_ref v = argv[1];
  Object* o = v.as<Object>();
  u16string n = ToString(c, argv[2]);
  auto it = o->own_props().find(n);
  if (it == o->own_props().end())
    return undefined_data::alloc();
  Property* desc = &(*it).second;
  if (!(desc->flags & Property::kAccessor)) {
    // IsDetaDescriptor(desc) is true
    Object* robj = Object::Alloc(c->object_proto());
    robj->Put(c, "value", desc->value_or_get, false);
    robj->Put(c, "writable", true, false);
    robj->Put(c, "enumerable", true, false);
    robj->Put(c, "configurable", true, false);
    return robj;
  } else {
    // IsAccessorDescriptor(desc) is true
    Object* robj = Object::Alloc(c->object_proto());
    any_ref get = desc->value_or_get;
    if (!get) get = undefined_data::alloc();
    any_ref set = desc->set;
    if (!set) set = undefined_data::alloc();
    robj->Put(c, "get", get, false);
    robj->Put(c, "set", set, false);
    robj->Put(c, "enumerable", true, false);
    robj->Put(c, "configurable", true, false);
    return robj;
  }

  return undefined_data::alloc();
}

static any_ref Object_defineProperty(Context* c, size_t argc, const any_ref* argv);

static any_ref Object_create(Context* c, size_t argc, const any_ref* argv) {
  // 15.2.3.5 Object.create ( O [, Properties] )
  if (!argv[0]) return ThrowTypeError(c);
  if (argc < 2) return ThrowTypeError(c);
  any_ref v = argv[1];
  Object* robj;
  if (v.is_null()) {
    robj = Object::Alloc(c->object_proto());
  } else if (v.is<Object>()) {
    robj = Object::Alloc(v.as<Object>());
  } else {
    return ThrowTypeError(c);
  }
  if (argc >= 3 && !argv[2].is_undefined()) {
    any_ref args[3];
    args[0] = argv[0];
    args[1] = robj;
    args[2] = argv[2];
    Object_defineProperty(c, 3, args);
  }
  return robj;
}

static any_ref Object_defineProperty(Context* c, size_t argc, const any_ref* argv) {
  // 15.2.3.6 Object.defineProperty ( O, P, Attributes )
  if (!argv[0]) return ThrowTypeError(c);
  if (argc < 4) return ThrowTypeError(c);
  if (!argv[1].is<Object>()) return ThrowTypeError(c);
  Object* o = argv[1].as<Object>();
  u16string n = ToString(c, argv[2]);
  if (!n) return nullptr;
  if (!argv[3].is<Object>()) return ThrowTypeError(c);
  Object* desc_obj = argv[3].as<Object>();
  Property* desc = o->NewOwnProperty(n);
  any_ref value_val = desc_obj->Get("value");
  if (!(value_val.is_undefined() || value_val.is_null())) {
    // IsDetaDescriptor(desc) is true
    desc->value_or_get = value_val;
    desc->flags = Property::kEnumerable | Property::kConfigurable;
  } else {
    // IsAccessorDescriptor(desc) is true
    desc->value_or_get = desc_obj->Get("get");
    desc->set = desc_obj->Get("set");
    desc->flags = Property::kAccessor | Property::kEnumerable | Property::kConfigurable;
  }

  return o;
}

static any_ref Object_keys(Context* c, size_t argc, const any_ref* argv) {
  // 15.2.3.14 Object.keys ( O )
  if (!argv[0]) return ThrowTypeError(c);
  if (argc < 2) return ThrowTypeError(c);
  any_ref v = argv[1];
  if (!v.is<Object>()) return ThrowTypeError(c);
  Object* o = v.as<Object>();
  Object* robj = NewArrayObject(c);
  int i = 0;
  for (auto it = o->own_props().begin(); it != o->own_props().end(); ++it) {
    Property* desc = &(*it).second;
    if (desc->flags & Property::kEnumerable) {
      u16string n = (*it).first;
      robj->Put(c, i++, n, false);
    }
  }
  return robj;
}

static any_ref Object_prototype_toString(Context* c, size_t argc, const any_ref* argv) {
  // 15.2.4.2 Object.prototype.toString ( )
  if (!argv[0]) return ThrowTypeError(c);
  any_ref this_val = argv[0];
  if (this_val.is_undefined()) return "[object Undefined]";
  if (this_val.is_null()) return "[object Null]";
  Object* this_obj = ToObject(c, this_val);
  any_ref data = this_obj->host_data;
  const char* class_name;
  if (!data) {
    class_name = "Object";
  } else if (data.is<Function>()) {
    class_name = "Function";
  } else if (data.is<Array>()) {
    class_name = "Array";
  } else if (data.is<RegExp>()) {
    class_name = "RegExp";
  } else if (data.is_bool()) {
    class_name = "Boolean";
  } else if (data.is_smi() || data.is_double()) {
    class_name = "Number";
  } else if (data.is_u16string()) {
    class_name = "String";
  } else {
    class_name = "Object";
  }
  return u16string("[object ") + class_name + "]";
}

static any_ref Object_prototype_valueOf(Context* c, size_t argc, const any_ref* argv) {
  // 15.2.4.4 Object.prototype.valueOf ( )
  if (!argv[0]) return ThrowTypeError(c);
  Object* this_obj = ToObject(c, argv[0]);
  if (!this_obj) return nullptr;
#if 0
  any_ref data = this_obj->host_data;
  if (!data) return this_obj;
  if (data.is_smi() || data.is_u16string() || data.is_bool() || data.is_double()) return data;
#endif
  return this_obj;
}

static any_ref Object_prototype_hasOwnProperty(Context* c, size_t argc, const any_ref* argv) {
  // 15.2.4.5 Object.prototype.hasOwnProperty ( )
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  Object* this_obj = argv[0].as<Object>();
  any_ref v = argv[1];
  u16string n = ToString(c, v);
  auto it = this_obj->own_props().find(n);
  return it != this_obj->own_props().end();
}

// 15.3 Function Objects

static any_ref Function_construct(Context* c, size_t argc, const any_ref* argv) {
  Object* this_obj;
  if (!argv[0])
    this_obj = Object::Alloc(c->function_proto());
  else
    this_obj = argv[0].as<Object>();
  if (!this_obj) this_obj = Object::Alloc(c->function_proto());
  return this_obj;
}

static any_ref Function_prototype_toString(Context* c, size_t argc, const any_ref* argv) {
  // 15.3.4.2 Function.prototype.toString ( )
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  Object* this_obj = argv[0].as<Object>();
  if (!this_obj->host_data.is<Function>()) return ThrowTypeError(c);
  u16string ret("function ");
  any_ref name_val = this_obj->Get("name");
  if (name_val.is_u16string()) ret = ret + name_val.as<u16string_data>();
  ret = ret + "() {";
  Function* fn = this_obj->host_data.as<Function>();
  if (fn->native_code) {
    ret = ret + " [native code]";
  } else {
    ret = ret + " ...";
  }
  return ret + " }";
}

static any_ref Function_prototype_apply(Context* c, size_t argc, const any_ref* argv) {
  // 15.3.4.3 Function.prototype.apply (thisArg, argArray)
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  Object* this_obj = argv[0].as<Object>();
  if (!IsCallable(this_obj)) return ThrowTypeError(c);
  any_ref this_arg;
  any_ref arg_array;

  if (argc < 2) {
    this_arg = undefined_data::alloc();
  } else {
    this_arg = argv[1];
  }
  if (argc < 3) {
    arg_array = undefined_data::alloc();
  } else {
    arg_array = argv[2];
  }

  if (arg_array.is_undefined() || arg_array.is_null()) {
    return this_obj->Call(1, &this_arg);
  }

  if (!arg_array.is<Object>()) return ThrowTypeError(c);
  Object* arg_array_obj = arg_array.as<Object>();

  any_ref len_val = arg_array_obj->Get("length");
  if (!len_val) return nullptr;
  uint32_t len;
  if (!ToInteger<uint32_t>(c, len_val, len)) return nullptr;
  // Too long arg array.
  if (len > 1000) return ThrowTypeError(c);
  
  any_vector arg_list;
  arg_list.init();
  arg_list.push_back(this_arg);
  for (uint32_t i = 0; i < len; i++) {
    any_ref v = arg_array_obj->Get(i);
    arg_list.push_back(v);
  }

  return this_obj->Call(arg_list.size(), arg_list.data());
}

static any_ref Function_prototype_call(Context* c, size_t argc, const any_ref* argv) {
  // 15.3.4.4 Function.prototype.call (thisArg [ , arg1 [ , arg2, ... ] ] )
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  Object* this_obj = argv[0].as<Object>();
  if (!IsCallable(this_obj)) return ThrowTypeError(c);
  any_ref this_arg;
  auto it = &argv[1];
  if (it != &argv[argc]) {
    this_arg = *it;
    ++it;
  } else {
    this_arg = undefined_data::alloc();
  }
  if (this_arg.is_undefined() || this_arg.is_null()) {
    this_arg = c->global_obj();
  }
  any_vector arg_list;
  arg_list.init();
  arg_list.push_back(ToObject(c, this_arg));
  while (it != &argv[argc]) {
    arg_list.push_back(*it);
    ++it;
  }
  return this_obj->Call(arg_list.size(), arg_list.data());
}

// 15.4 Array Objects

static any_ref Array_construct(Context* c, size_t argc, const any_ref* argv) {
  // 15.4.2 The Array Constructor
  if (argc == 2) {
    // 15.4.2.2 new Array (len)
    uint32_t n;
    if (!ToInteger<uint32_t>(c, argv[1], n)) return nullptr;
    return NewArrayObject(c, n);
  } else {
    // 15.4.2.1 new Array ( [ item0 [ , item1 [ , �c ] ] ] )
    return NewArrayObject(c, argc - 1, argv + 1);
  }
}

static any_ref Array_isArray(Context* c, size_t argc, const any_ref* argv) {
  // 15.4.3.2 Array.isArray ( arg )
  if (!argv[0]) return ThrowTypeError(c);
  if (argc < 2) return false;
  any_ref this_val = argv[1];
  if (!this_val.is<Object>()) return false;
  Object* this_obj = this_val.as<Object>();
  if (!this_obj->host_data) return false;
  if (!this_obj->host_data.is<Array>()) return false;
  return true;
}

static any_ref Array_prototype_concat(Context* c, size_t argc, const any_ref* argv) {
  // 15.4.4.4 Array.prototype.concat ( [ item1 [ , item2 [ , … ] ] ] )
  if (!argv[0]) return ThrowTypeError(c);
  Object* robj = NewArrayObject(c);
  uint32_t n = 0;
  for (size_t i = 0; i < argc; i++) {
    any_ref v = argv[i];
    if (i == 0) {
      Object* o = ToObject(c, argv[0]);
      if (!o) return nullptr;
      v = o;
    }
    if (v.is<Object>()) {
      Object* o = v.as<Object>();
      if (!!o->host_data && o->host_data.is<Array>()) {
        Array *a = o->host_data.as<Array>();
        uint32_t len = a->length;
        for (uint32_t j = 0; j < len; j++) {
          any_ref v = o->Get(j);
          if (!v) return nullptr;
          if (!robj->Put(c, n++, v, false)) return nullptr;
        }
      } else {
        if (!robj->Put(c, n++, v, false)) return nullptr;
      }
    } else {
      if (!robj->Put(c, n++, v, false)) return nullptr;
    }
  }
  return robj;
}

static any_ref Array_prototype_forEach(Context* c, size_t argc, const any_ref* argv) {
  // 15.4.4.18 Array.prototype.forEach ( callbackfn [ , thisArg ] )
  if (!argv[0]) return ThrowTypeError(c);
  Object* this_obj = ToObject(c, argv[0]);
  if (!this_obj) return nullptr;

  any_ref len_val = this_obj->Get("length");
  if (!len_val) return nullptr;
  uint32_t len;
  if (!ToInteger<uint32_t>(c, len_val, len)) return nullptr;

  if (argc < 2 || !argv[1].is<Object>()) return ThrowTypeError(c);
  Object* func_obj = argv[1].as<Object>();
  if (!IsCallable(func_obj)) return ThrowTypeError(c);

  any_ref this_arg;
  if (argc < 3) {
    this_arg = undefined_data::alloc();
  } else {
    this_arg = ToObject(c, argv[2]);
    if (!this_arg) return nullptr;
  }
      
  for (uint32_t i = 0; i < len; i++) {
    Property* desc = this_obj->GetProperty(i);
    if (!desc) continue;
    any_ref v = this_obj->Get(desc);
    if (!v) return nullptr;
    any_ref argv[4];
    argv[0] = this_arg;
    argv[1] = v;
    argv[2] = static_cast<double>(i);
    argv[3] = this_obj;
    if (!func_obj->Call(4, argv)) return nullptr;
  }
  return undefined_data::alloc();
}

static any_ref Array_prototype_pop(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  Object* this_obj = argv[0].as<Object>();
  if (!this_obj->host_data || !this_obj->host_data.is<Array>())
    return ThrowTypeError(c);
  Array* arr = this_obj->host_data.as<Array>();
  if (arr->length == 0) return undefined_data::alloc();
  any_ref v = this_obj->Get(arr->length - 1);
  if (!this_obj->Put(c, "length", static_cast<double>(arr->length - 1), true))
    return nullptr;
  return v;
}

static any_ref Array_prototype_splice(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);

  Object* this_obj = ToObject(c, argv[0]);
  if (!this_obj) return nullptr;
  any_ref array_length_val = this_obj->Get("length");
  uint32_t array_length;
  if (!ToInteger<uint32_t>(c, array_length_val, array_length)) return nullptr;

  uint32_t start = 0;
  if (argc >= 2) {
    int32_t n;
    if (!ToInteger<int32_t>(c, argv[1], n)) return nullptr;
    if (n < 0) {
      n += array_length;
      if (n < 0) n = 0;
      start = static_cast<uint32_t>(n);
    } else {
      start = static_cast<uint32_t>(n);
      if (start > array_length) start = array_length;
    }
  }
  uint32_t delete_count = 0;
  if (argc >= 3) {
    int32_t n;
    if (!ToInteger<int32_t>(c, argv[2], n)) return nullptr;
    if (n < 0) n = 0;
    delete_count = static_cast<uint32_t>(n);
    if (delete_count > array_length - start) delete_count = array_length - start;
  }
  uint32_t insert_count = argc - 3;
  Object* robj = NewArrayObject(c);
  for (uint32_t i = 0; i < delete_count; i++) {
    any_ref v = this_obj->Get(start + i);
    robj->Put(c, i, v, true);
  }
  if (delete_count >= insert_count) {
    uint32_t diff = delete_count - insert_count;
    for (uint32_t i = 0; i < insert_count; i++) {
      this_obj->Put(c, start + i, argv[i + 3], true);
    }
    for (uint32_t i = start + delete_count; i < array_length; i++) {
      any_ref v = this_obj->Get(i);
      this_obj->Put(c, i - diff, v, true);
    }
    for (uint32_t i = array_length - diff; i < array_length; i++) {
      this_obj->Delete(c, i, true);
    }
    this_obj->Put(c, "length", static_cast<double>(array_length - diff), false);
  } else {
    uint32_t diff = insert_count - delete_count;
    for (uint32_t i = array_length + diff - 1; i >= start + insert_count; i--) {
      any_ref v = this_obj->Get(i - diff);
      this_obj->Put(c, i, v, false);
    }
    for (uint32_t i = 0; i < insert_count; i++) {
      any_ref v = argv[i + 3];
      this_obj->Put(c, start + i, v, true);
    }
    this_obj->Put(c, "length", static_cast<double>(array_length + diff), true);
  }
  return robj;
}

// 15.5 String Objects

static any_ref String_construct(Context* c, size_t argc, const any_ref* argv) {
  u16string str;
  if (argc < 2) {
    str = "";
  } else {
    str = ToString(c, argv[1]);
    if (!str) return nullptr;
  }

  if (!argv[0]) {
    // 15.5.2.1 new String ( [ value ] )
    return NewStringObject(c, str);
  } else {
    // 15.5.1.1 String ( [ value ] )
    return str;
  }
}

static any_ref String_fromCharCode(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  char16_t buf[] = { 0 };
  if (argc - 1 >= 1) {
    any_ref v = argv[1];
    if (!ToInteger<char16_t>(c, v, buf[0])) return nullptr;
  }
  return u16string(buf, 1);
}

static any_ref String_prototype_charCodeAt(Context* c, size_t argc, const any_ref* argv) {
  // 15.5.4.5 String.prototype.charCodeAt (pos)
  if (!argv[0]) return ThrowTypeError(c);
  if (!CheckObjectCoercible(c, argv[0])) return nullptr;
  u16string s = ToString(c, argv[0]);
  int position = 0;
  if (argc >= 2) {
    if (!ToInteger<int>(c, argv[1], position)) return nullptr;
  }
  if (position < 0 || (size_t)position >= s.length()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return (int)(s.data()[position]);
}

static any_ref String_prototype_substring(Context* c, size_t argc, const any_ref* argv) {
  // 15.5.4.15 String.prototype.substring (start, end)
  if (!argv[0]) return ThrowTypeError(c);
  if (!CheckObjectCoercible(c, argv[0])) return nullptr;
  u16string s = ToString(c, argv[0]);
  int len = static_cast<int>(s.length());
  int start, end;
  if (argc < 2) {
    start = 0;
  } else {
    if (!ToInteger<int>(c, argv[1], start)) return nullptr;
  }
  if (argc < 3 || argv[2].is_undefined()) {
    end = len;
  } else {
    if (!ToInteger(c, argv[2], end)) return nullptr;
  }
  start = start < 0 ? 0 : start < len ? start : len;
  end = end < 0 ? 0 : end < len ? end : len;
  if (start > end) {
    int t = end;
    end = start;
    start = t;
  }
  return u16string(s.data() + start, end - start);
}

static any_ref String_prototype_indexOf(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  if (!CheckObjectCoercible(c, argv[0])) return nullptr;
  u16string s = ToString(c, argv[0]);
  u16string search_str;
  if (argc < 2) {
    search_str = "undefined";
  } else {
    search_str = ToString(c, argv[1]);
  }
  int pos;
  if (argc < 3) {
    pos = 0;
  } else {
    if (!ToInteger(c, argv[2], pos)) return nullptr;
  }
  int end = s.length() - search_str.length();
  int start = pos < 0 ? 0 : pos;
  while (start < end) {
    const char16_t* sp = s.data() + start;
    const char16_t* dp = search_str.data();
    int n = search_str.length();
    while (n > 0) {
      if (*sp++ != *dp++) break;
      n--;
    }
    if (n == 0) return start;
    start++;
  }
  return -1;
}

static any_ref String_prototype_lastIndexOf(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  if (!CheckObjectCoercible(c, argv[0])) return nullptr;
  u16string s = ToString(c, argv[0]);
  u16string search_str;
  if (argc < 2) {
    search_str = "undefined";
  } else {
    search_str = ToString(c, argv[1]);
  }
  int pos;
  if (argc < 3) {
    pos = 0;
  } else {
    if (!ToInteger(c, argv[2], pos)) return nullptr;
  }
  int start = s.length() - search_str.length();
  if (pos < start) start = pos;
  while (start >= 0) {
    const char16_t* sp = s.data() + start;
    const char16_t* dp = search_str.data();
    int n = search_str.length();
    while (n > 0) {
      if (*sp++ != *dp++) break;
      n--;
    }
    if (n == 0) return start;
    start--;
  }
  return -1;
}
  
static any_ref String_prototype_search(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  if (!CheckObjectCoercible(c, argv[0])) return nullptr;
  u16string s = ToString(c, argv[0]);
  if (!s) return nullptr;
  u16string pstr;
  if (argc >= 2) {
    any_ref v = argv[1];
    if (v.is<Object>()) {
      Object* o = v.as<Object>();
      if (o->proto() == c->regexp_proto()) {
        v = o->Get("source");
        if (!v) return nullptr;
      }
    }
    pstr = ToString(c, v);
    if (!pstr) return nullptr;
  } else {
    pstr = " ";
  }

  const char* error;
  int erroffset;
  std::u16string p(pstr.begin(), pstr.end());
  pcre16* re = pcre16_compile(reinterpret_cast<PCRE_SPTR16>(p.c_str()), 0, &error, &erroffset, NULL);
  assert(re);
  int ovector[30];
  int rc = pcre16_exec(re, NULL, reinterpret_cast<PCRE_SPTR16>(s.data()), s.length(), 0, 0, ovector, 30);
  int result;
  if (rc > 0) {
    result = ovector[0];
  } else {
    result = -1;
  }
  pcre16_free(re);

  return result;
}

static any_ref String_prototype_toLowerCase(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  if (!CheckObjectCoercible(c, argv[0])) return nullptr;
  u16string str = ToString(c, argv[0]);
  if (!str) return nullptr;
  size_t len = str.length();
  const char16_t* src = str.data();
  char16_t* dst = new char16_t[len];
  for (size_t i = 0; i < len; i++) {
    char16_t ch = src[i];
    if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
    dst[i] = ch;
  }
  str = u16string(dst, len);
  delete dst;
#if 0
  std::u16string u16str(str.data(), str.data() + str.length());
  std::transform(u16str.begin(), u16str.end(), u16str.begin(), ::tolower);
#endif
  return str;
}

static any_ref String_prototype_toUpperCase(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  if (!CheckObjectCoercible(c, argv[0])) return nullptr;
  u16string str = ToString(c, argv[0]);
  if (!str) return nullptr;
  size_t len = str.length();
  const char16_t* src = str.data();
  char16_t* dst = new char16_t[len];
  for (size_t i = 0; i < len; i++) {
    char16_t ch = src[i];
    if (ch >= 'a' && ch <= 'a') ch -= 'a' - 'A';
    dst[i] = ch;
  }
  str = u16string(dst, len);
  delete dst;
#if 0
  std::u16string u16str(str.data(), str.data() + str.length());
  std::transform(u16str.begin(), u16str.end(), u16str.begin(), ::tolower);
#endif
  return str;
}

static any_ref String_prototype_toString(Context* c, size_t argc, const any_ref* argv) {
  // 15.5.4.2 String.prototype.toString ( )
  if (!argv[0]) return ThrowTypeError(c);
  any_ref this_val = argv[0];
  if (this_val.is_u16string()) return this_val;
  if (!this_val.is<Object>()) return ThrowTypeError(c);
  Object* this_obj = this_val.as<Object>();
  if (!this_obj->host_data || !this_obj->host_data.is_u16string())
    return ThrowTypeError(c);
  return this_obj->host_data.as_u16string();
}

static any_ref String_prototype_valueOf(Context* c, size_t argc, const any_ref* argv) {
  // 15.5.4.3 String.prototype.valueOf ( )
  return String_prototype_toString(c, argc, argv);
}

// 15.6 Boolean Objects

static any_ref Boolean_construct(Context* c, size_t argc, const any_ref* argv) {
  bool bval;
  if (argc < 2) {
    bval = false;
  } else {
    bval = ToBoolean(argv[1]);
  }

  if (!argv[0]) {
    // 15.6.2.1 new Boolean (value)
    Object* this_obj = Object::Alloc(c->boolean_proto());
    this_obj->host_data = bval;
    return this_obj;
  } else {
    // 15.6.1.1 Boolean (value)
    return bval;
  }
}

static any_ref Boolean_prototype_valueOf(Context* c, size_t argc, const any_ref* argv) {
  // 15.6.4.3 Boolean.prototype.valueOf ( )
  if (!argv[0]) return ThrowTypeError(c);
  any_ref this_val = argv[0];
  if (this_val.is_bool()) return this_val;
  if (!this_val.is<Object>()) return ThrowTypeError(c);
  Object* this_obj = this_val.as<Object>();
  if (!this_obj->host_data || !this_obj->host_data.is_bool())
    return ThrowTypeError(c);
  return this_obj->host_data;
}

// 15.7 Number Objects

static any_ref Number_construct(Context* c, size_t argc, const any_ref* argv) {
  double dval;
  if (argc < 2) {
    dval = 0;
  } else {
    if (!ToNumber(c, argv[1], dval)) return nullptr;
  }

  if (!argv[0]) {
    // 15.7.2.1 new Number ( [ value ] )
    Object* this_obj = Object::Alloc(c->number_proto());
    this_obj->host_data = dval;
    return this_obj;
  } else {
    // 15.7.1.1 Number ( [ value ] )
    return dval;
  }
}

static any_ref Number_prototype_valueOf(Context* c, size_t argc, const any_ref* argv) {
  // 15.7.4.4 Number.prototype.valueOf ( )
  if (!argv[0]) return ThrowTypeError(c);
  any_ref this_val = argv[0];
  if (this_val.is_smi() || this_val.is_double()) return this_val;
  if (!this_val.is<Object>()) return ThrowTypeError(c);
  Object* this_obj = this_val.as<Object>();
  if (!this_obj->host_data || !(this_obj->host_data.is_smi() || this_obj->host_data.is_double()))
    return ThrowTypeError(c);
  return this_obj->host_data;
}

// 15.8 The Math Object

static any_ref Math_floor(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  any_ref v = GET_ARG(1);
  if (v.is_smi()) {
    return v.smi();
  } else {
    double num;
    if (!ToNumber(c, v, num)) return nullptr;
    return static_cast<double>(std::floor(num));
  }
}

static any_ref Math_log(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  any_ref xval = GET_ARG(1);
  double x;
  if (!ToNumber(c, xval, x)) return nullptr;
  return std::log(x);
}

static any_ref Math_pow(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  any_ref xval = GET_ARG(1);
  any_ref yval = GET_ARG(2);
  double x, y;
  if (!ToNumber(c, xval, x)) return nullptr;
  if (!ToNumber(c, yval, y)) return nullptr;
  return std::pow(x, y);
}

static any_ref Math_random(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0]) return ThrowTypeError(c);
  return static_cast<double>(rand()) / RAND_MAX;
}

// 15.9 Date Objects

static any_ref Date_construct(Context* c, size_t argc, const any_ref* argv) {
  Object* this_obj;
  uint64_t tval;
  if (!!argv[0] || argc <= 1) {
    // 15.9.2.1 Date ( [ year [, month [, date [, hours [, minutes [, seconds [, ms ] ] ] ] ] ] ] )
    // 15.9.3.3 new Date ( )
#if 1
    using namespace std::chrono;
    auto dtn = system_clock::now().time_since_epoch();
    tval = duration_cast<milliseconds>(dtn).count();
#else
    tval = time(nullptr);
#endif
  } else if (argc <= 2) {
    // 15.9.3.2 new Date (value)
    any_ref v = ToPrimitive(c, argv[1]);
    if (!v) return nullptr;
    if (v.is_smi()) {
      tval = v.smi();
    } else if (v.is_u16string()) {
      // TODO:
      assert(false);
      tval = 0;
    } else {
      if (!ToInteger<uint64_t>(c, v, tval)) return nullptr;
    }
  } else {
    // 15.9.3.1 new Date (year, month [, date [, hours [, minutes [, seconds [, ms ] ] ] ] ] )
    // TODO:
    assert(false);
    tval = 0;
  }

  this_obj = Object::Alloc(c->date_proto());
  Date* date = Date::Alloc();
  date->value = tval;
  this_obj->host_data = date;

  if (!argv[0]) {
    return this_obj;
  } else {
    return ToString(c, this_obj);
  }
}

static any_ref Date_prototype_getTime(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  Object* this_obj = argv[0].as<Object>();
  if (!this_obj->host_data.is<Date>()) return ThrowTypeError(c);
  Date* date = this_obj->host_data.as<Date>();
  double dval = static_cast<double>(date->value);
  return double_data::alloc(dval);
}

static any_ref Date_prototype_toString(Context* c, size_t argc, const any_ref* argv) {
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  return "[date]";
}

// 15.10 RegExp (Regular Expression) Objects

RegExp* RegExp::Alloc() {
  RegExp* re = reinterpret_cast<RegExp*>(GC_MALLOC(sizeof (RegExp)));
  re->tag_ = kTagRegExp;
  GC_REGISTER_FINALIZER(re, [] (GC_PTR obj, GC_PTR client_data) {
    RegExp* re = reinterpret_cast<RegExp*>(obj);
    pcre16_free(re->re);
  }, 0, NULL, NULL);
  return re;
}

Object* NewRegExpObject(Context* c, u16string pattern_str, u16string flags_str) {
  Object* this_obj = Object::Alloc(c->regexp_proto());
  RegExp* data = RegExp::Alloc();
  this_obj->host_data = data;

  const char* error;
  int erroffset;
  std::u16string p(pattern_str.begin(), pattern_str.end());
  data->re = pcre16_compile(reinterpret_cast<PCRE_SPTR16>(p.c_str()), 0, &error, &erroffset, NULL);
  if (!data->re) return ThrowTypeError(c);
  this_obj->DefineOwnDataPropertyNoCheck("source", pattern_str, Property::kNone);

  int flags = 0;
  for (auto it = flags_str.begin(); it != flags_str.end(); ++it) {
    char16_t ch = *it;
    if (ch == 'g') {
      if (flags & RegExp::kGlobal) return ThrowSyntaxError(c);
      flags |= RegExp::kGlobal;
    } else if (ch == 'i') {
      if (flags & RegExp::kIgnoreCase) return ThrowSyntaxError(c);
      flags |= RegExp::kIgnoreCase;
    } else if (ch == 'm') {
      if (flags & RegExp::kMultiline) return ThrowSyntaxError(c);
      flags |= RegExp::kMultiline;
    } else {
      return ThrowSyntaxError(c);
    }
  }
  data->flags = flags;
  
  this_obj->DefineOwnDataPropertyNoCheck("global", !!(flags & RegExp::kGlobal), Property::kNone);
  this_obj->DefineOwnDataPropertyNoCheck("ignoreCase", !!(flags & RegExp::kIgnoreCase), Property::kNone);
  this_obj->DefineOwnDataPropertyNoCheck("multiline", !!(flags & RegExp::kMultiline), Property::kNone);
  this_obj->DefineOwnDataPropertyNoCheck("lastIndex", 0, Property::kWritable);

  return this_obj;
}

any_ref RegExp_construct(Context* c, size_t argc, const any_ref* argv) {
  if (!!argv[0]) {
    // 15.10.3.1 RegExp(pattern, flags)
    if (argc > 2 && argv[1].is<Object>()) {
      Object* robj = argv[1].as<Object>();
      if (robj->host_data.as<RegExp>()) {
        if (argc < 3 || argv[2].is_undefined()) {
          return robj;
        }
      }
    }
  }

  // 15.10.4.1 new RegExp(pattern, flags)

  u16string pattern_str;
  if (argc < 2 || argv[1].is_undefined()) {
    pattern_str = "";
  } else {
    pattern_str = ToString(c, argv[1]);
    if (!pattern_str) return nullptr;
  }

  u16string flags_str;
  if (argc < 3 || argv[2].is_undefined()) {
    flags_str = "";
  } else {
    flags_str = ToString(c, argv[2]);
    if (!flags_str) return nullptr;
  }

  return NewRegExpObject(c, pattern_str, flags_str);
}

static any_ref RegExp_prototype_exec(Context* c, size_t argc, const any_ref* argv) {
  // 15.10.6.2 RegExp.prototype.exec(string)
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  Object* this_obj = argv[0].as<Object>();
  if (!this_obj->host_data.is<RegExp>()) return ThrowTypeError(c);
  RegExp* re = this_obj->host_data.as<RegExp>();
  u16string str = ToString(c, GET_ARG(1));
  any_ref lastindex_val = this_obj->Get("lastIndex");
  uint32_t lastindex;
  if (!ToInteger<uint32_t>(c, lastindex_val, lastindex)) return nullptr;
  int ovector[30];
  int rc = pcre16_exec(re->re, NULL,
                       reinterpret_cast<PCRE_SPTR16>(str.data()),
                       str.length(), lastindex, 0, ovector, 30);
  if (rc <= 0) return null_data::alloc();

  Object* robj = NewArrayObject(c);
  robj->Put(c, "index", static_cast<double>(ovector[0]), true);
  robj->Put(c, "input", str, true);
  robj->Put(c, "length", rc, true);
  for (int i = 0; i < rc; i++) {
    int startindex = ovector[i * 2];
    int endindex = ovector[i * 2 + 1];
    robj->Put(c, i, u16string(str.data() + startindex, endindex - startindex), true);
  }
  return robj;
}

// 15.11 Error Objects

static any_ref Error_construct(Context* c, size_t argc, const any_ref* argv) {
  // 15.11.1.1 Error (message)
  Object* this_obj;
  if (!argv[0])
    this_obj = Object::Alloc(c->error_proto());
  else
    this_obj = argv[0].as<Object>();
  if (argc - 1 >= 1) {
    any_ref msg_val = argv[1];
    if (!msg_val.is_undefined()) {
      u16string msg_str = ToString(c, msg_val);
      this_obj->Put(c, "message", msg_str, false);
    }
  }
  return this_obj;
}

static any_ref Error_prototype_toString(Context* c, size_t argc, const any_ref* argv) {
  // 15.11.4.4 Error.prototype.toString ( )
  if (!argv[0] || !argv[0].is<Object>()) return ThrowTypeError(c);
  Object* this_obj = argv[0].as<Object>();
  any_ref name_val = this_obj->Get("name");
  u16string name_str;
  if (name_val.is_undefined()) {
    name_str = "Error";
  } else {
    name_str = ToString(c, name_val);
  }
  any_ref msg_val = this_obj->Get("message");
  u16string msg_str;
  if (msg_val.is_undefined()) {
    msg_str = "";
  } else {
    msg_str = ToString(c, msg_val);
  }
  if (name_str.length() == 0)
    return msg_str;
  if (msg_str.length() == 0)
    return name_str;
  return name_str + ": " + msg_str;
}

static Object* make_global_object(Context* c, const func_spec* table);
static bool make_builtin_object(Context* c, Object* global, const char* name, NativeCodeProc constructor, const func_spec* table, const func_spec* proto_table, Object** proto_prt = nullptr);

static func_spec global_funcs[] = {
  { "eval", Global_eval },
  { "parseFloat", Global_parseFloat },
  { nullptr, nullptr }
};

static func_spec ext_funcs[] = {
  { "evalcx", Global_evalcx },
  { "load", Global_load },
  { "read", Global_read },
  { "print", Global_print },
  { "quit", Glolbal_quit },
  { nullptr, nullptr }
};

static func_spec object_funcs[] = {
  { "create", Object_create },
  { "defineProperty", Object_defineProperty },
  { "getOwnPropertyDescriptor", Object_getOwnPropertyDescriptor },
  { "getPrototypeOf", Object_getPrototypeOf },
  { "keys", Object_keys },
  { nullptr, nullptr }
};

static func_spec object_prototype_funcs[] = {
  { "valueOf", Object_prototype_valueOf },
  { "hasOwnProperty", Object_prototype_hasOwnProperty },
  { "toString", Object_prototype_toString },
  { nullptr, nullptr }
};

static func_spec function_prototype_funcs[] = {
  { "toString", Function_prototype_toString },
  { "apply", Function_prototype_apply },
  { "call", Function_prototype_call },
  { nullptr, nullptr }
};

static func_spec array_funcs[] = {
  { "isArray", Array_isArray },
  { nullptr, nullptr }
};

static func_spec array_prototype_funcs[] = {
  { "concat", Array_prototype_concat },
  { "forEach", Array_prototype_forEach },
  { "splice", Array_prototype_splice },
  { "pop", Array_prototype_pop },
  { nullptr, nullptr }
};

static func_spec string_funcs[] = {
  { "fromCharCode", String_fromCharCode }
};

static func_spec string_prototype_funcs[] = {
  { "charCodeAt", String_prototype_charCodeAt },
  { "indexOf", String_prototype_indexOf },
  { "lastIndexOf", String_prototype_lastIndexOf },
  { "search", String_prototype_search },
  { "substring", String_prototype_substring },
  { "toLowerCase", String_prototype_toLowerCase },
  { "toUpperCase", String_prototype_toUpperCase },
  { "toString", String_prototype_toString },
  { "valueOf", String_prototype_valueOf },
  { nullptr, nullptr }
};

static func_spec boolean_object_funcs[] = {
  { nullptr, nullptr }
};

static func_spec boolean_prototype_funcs[] = {
  { "valueOf", Boolean_prototype_valueOf },
  { nullptr, nullptr }
};

static func_spec number_object_funcs[] = {
  { nullptr, nullptr }
};

static func_spec number_prototype_funcs[] = {
  { "valueOf", Number_prototype_valueOf },
  { nullptr, nullptr }
};

static func_spec math_funcs[] = {
  { "floor", Math_floor },
  { "log", Math_log },
  { "pow", Math_pow },
  { "random", Math_random },
  { nullptr, nullptr }
};

static func_spec date_prototype_funcs[] = {
  { "getTime", Date_prototype_getTime },
  { "toString", Date_prototype_toString },
  { nullptr, nullptr }
};

static func_spec regexp_prototype_funcs[] = {
  { "exec", RegExp_prototype_exec },
  { nullptr, nullptr }
};

static func_spec error_prototype_funcs[] = {
  { "toString", Error_prototype_toString },
  { nullptr, nullptr }
};

void Context::InitStandardBuiltInObjects() {
  // Create Function prototype before everything so that we can
  // create other functions.
  object_proto_ = Object::Alloc(nullptr);
  function_proto_ = Object::Alloc(object_proto_);
  {
    Object* o = Object::Alloc(object_proto());
    Property* desc = o->NewOwnProperty("length");
    desc->flags = 0;
    desc->value_or_get = 0;
    o->host_data = "";
    string_proto_ = o;
  }
  {
    Object* o = Object::Alloc(object_proto());
    Property* desc = o->NewOwnProperty("length");
    desc->flags = Property::kWritable | Property::kEnumerable;
    desc->value_or_get = 0;
    o->host_data = Array::Alloc();
    array_proto_ = o;
  }

  global_obj_ = make_global_object(this, global_funcs);
  make_builtin_object(this, global_obj_, "Object",   Object_construct,   object_funcs,         object_prototype_funcs,   &object_proto_);
  make_builtin_object(this, global_obj_, "Function", Function_construct, nullptr,              function_prototype_funcs, &function_proto_);
  make_builtin_object(this, global_obj_, "Boolean",  Boolean_construct,  boolean_object_funcs, boolean_prototype_funcs,  &boolean_proto_);
  make_builtin_object(this, global_obj_, "Number",   Number_construct,   number_object_funcs,  number_prototype_funcs,   &number_proto_);
  make_builtin_object(this, global_obj_, "String",   String_construct,   string_funcs,         string_prototype_funcs,   &string_proto_);
  make_builtin_object(this, global_obj_, "Array",    Array_construct,    array_funcs,          array_prototype_funcs,    &array_proto_);
    
  make_builtin_object(this, global_obj_, "RegExp",   RegExp_construct,   nullptr,              regexp_prototype_funcs,   &regexp_proto_);
  make_builtin_object(this, global_obj_, "Math",     nullptr,            math_funcs,           nullptr);
  make_builtin_object(this, global_obj_, "Date",     Date_construct,     nullptr,              date_prototype_funcs,     &date_proto_);
  make_builtin_object(this, global_obj_, "Error",    Error_construct,    nullptr,              error_prototype_funcs,    &error_proto_);

  error_proto_->Put(this, "name", "Error", false);
  error_proto_->Put(this, "message", "", false);
}

static bool ExtendObjectWithNativeFunctions(Context* c, Object* o, const func_spec* table) {
  const func_spec* it = table;
  while (it->name) {
    if (!o->Put(c, it->name, CreateNativeFunction(c, it->native_code), false)) return false;
    it++;
  }
  return true;
}

void Context::InitExtendedBuiltInObjects() {
  ExtendObjectWithNativeFunctions(this, global_obj_, ext_funcs);
}

static Object* make_global_object(Context* c, const func_spec* table) {
  Object* o = Object::Alloc(c->object_proto());
  o->host_data = c;
  if (!ExtendObjectWithNativeFunctions(c, o, table))
    return nullptr;
  o->Put(c, "undefined", undefined_data::alloc(), false);
  double NaN = std::numeric_limits<double>::quiet_NaN();
  double Infinity = std::numeric_limits<double>::infinity();
  o->Put(c, "NaN", double_data::alloc(NaN), false);
  o->Put(c, "Infinity", double_data::alloc(Infinity), false);
  return o;
}

static bool make_builtin_object(Context* c, Object* global, const char* name, NativeCodeProc construct, const func_spec* table, const func_spec* proto_table, Object** proto_ptr) {
  Object* proto = proto_ptr ? *proto_ptr : nullptr;

  Object* constructor_obj;
  if (construct) {
    constructor_obj = CreateNativeFunction(c, construct);
    if (!proto) proto = Object::Alloc(c->object_proto());
    Property *desc = constructor_obj->NewOwnProperty("prototype");
    desc->flags = 0;
    desc->value_or_get = proto;
    
    if (!proto->Put(c, "constructor", constructor_obj, false))
      return false;
  } else {
    constructor_obj = Object::Alloc(c->object_proto());
  }

  if (table) {
    if (!ExtendObjectWithNativeFunctions(c, constructor_obj, table))
      return false;
  }

  if (proto_table) {
    if (!ExtendObjectWithNativeFunctions(c, proto, proto_table))
      return false;
  }

  if (!global->Put(c, name, constructor_obj, false))
    return false;
  if (proto_ptr) *proto_ptr = proto;
  return true;
}

}  // namespace internal
}  // namespace nabla
