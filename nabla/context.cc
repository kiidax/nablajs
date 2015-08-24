/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "context.hh"

#include <cmath>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <gc/gc.h>
#include <nabla/data.hh>

#include "ast.hh"
#include "evalast.hh"
#include "debug.hh"

namespace nabla {
namespace internal {

static uint32_t array_index(const char16_t *s, size_t n) {
  if (n == 0) return UINT32_MAX;
  int ch = *s++;
  if (ch == '0') return n == 1 ? 0 : UINT32_MAX;
  if (ch < '1' || ch > '9') return UINT32_MAX;
  int index = (ch - '0');
  while (--n) {
    ch = *s++;
    if (ch < '0' || ch > '9') return UINT32_MAX;
    index = index * 10 + (ch - '0');
  }
  return index;
}

static u16string uint32_to_u16string(uint32_t n) {
  char16_t buf[32];
  if (n < 10) {
    buf[0] = '0' + n;
    return u16string(buf, 1);
  }
  char16_t* p = &buf[32];
  do {
    uint32_t r = n % 10;
    n /= 10;
    *(--p) = '0' + r;
  } while (n > 0);
  return u16string(p, &buf[32] - p);
}

Object* Object::Alloc(Object* proto) {
  // 13.2.2 [[Construct]]
  Object* o = reinterpret_cast<Object*>(GC_MALLOC(sizeof (Object)));
  if (!o) return nullptr;
  o->tag_ = kTagObject;
  o->own_props_.init();
  o->proto_ = proto;
  o->flags = kExtensible;
  return o;
}

Property* Object::GetProperty(u16string n) {
  // 8.12.2 [[GetProperty]] (P)
  Object* o = this;
  do {
    auto it = o->own_props_.find(n);
    if (it != o->own_props_.end()) return &(*it).second;
    o = o->proto_;
  } while (o);
  return nullptr;
}

any_ref Object::Get(u16string n) {
  if (!!host_data && host_data.is_u16string()) {
    // 15.5.5.2 [[GetOwnProperty]] ( P )
    u16string s = host_data.as_u16string();
    uint32_t index = array_index(n.data(), n.length());
    if (index != UINT32_MAX) {
      if (index < s.length()) {
        return u16string(s.data() + index, 1);
      } else {
        return undefined_data::alloc();
      }
    }
  }

  Property* desc = GetProperty(n);
  if (!desc) return undefined_data::alloc();
  return Get(desc);
}

any_ref Object::Get(const Property* desc) {
  // 8.12.3 [[Get]] (P)
  if (!(desc->flags & Property::kAccessor)) {
    // IsDetaDescriptor(desc) is true
    return desc->value_or_get;
  }
  // IsAccessorDescriptor(desc) is true
  if (!desc->value_or_get)
    return undefined_data::alloc();
  Object* func = desc->value_or_get.as<Object>();
  any_ref this_val = this;
  return func->Call(1, &this_val);
}

any_ref Object::Get(uint32_t n) {
  u16string s = uint32_to_u16string(n);
  return Get(s);
}

Property* Object::GetProperty(uint32_t n) {
  u16string s = uint32_to_u16string(n);
  return GetProperty(s);
}

bool Object::DefineOwnArrayProperty(Context* c, u16string n, any_ref v, bool do_throw, Property* own_desc) {
  // 15.4.5.1 [[DefineOwnProperty]] ( P, Desc, Throw )
  Array* arr = host_data.as<Array>();
  if (n == "length") {
    double d;
    if (!ToNumber(c, v, d)) return false;
    uint32_t newlen = static_cast<uint32_t>(d);
    if (static_cast<double>(newlen) != d) return ThrowTypeError(c);
    uint32_t oldlen = arr->length;
    if (newlen < oldlen) {
      for (uint32_t i = oldlen; i < newlen; i++) {
        u16string s = uint32_to_u16string(i);
        Property* idx_desc = GetOwnProperty(s);
        idx_desc->value_or_get = undefined_data::alloc();
        idx_desc->flags = Property::kWritable | Property::kEnumerable | Property::kConfigurable;
      }
    }
    arr->length = newlen;
    v = static_cast<double>(newlen);
    own_desc->value_or_get = v;
    return true;
  } else {
    uint32_t index = array_index(n.data(), n.length());
    if (index != UINT32_MAX) {
      if (index >= arr->length) {
        arr->length = index + 1;
        Property* len_desc = GetOwnProperty("length");
        len_desc->value_or_get = (double)(index + 1);
      }
    }
    own_desc->value_or_get = v;
    return true;
  }
}

void Object::DefineOwnDataPropertyNoCheck(u16string n, any_ref v, int flags) {
  Property* desc = NewOwnProperty(n);
  desc->value_or_get = v;
  desc->flags = flags;
}

bool Object::Put(Context* c, u16string n, any_ref v, bool do_throw) {
  // 8.12.4 [[CanPut]] (P)
  // 8.12.5 [[Put]] ( P, V, Throw )
  Property* own_desc = GetOwnProperty(n);
  if (own_desc) {
    if (own_desc->flags & Property::kWritable) {
      // IsDataDescriptor(ownDesc) is true.
      // [[CanPut]] returns true.
      // Call [[DefineOwnProperty]]
      if (!!host_data && host_data.is<Array>()) {
        if (!DefineOwnArrayProperty(c, n, v, do_throw, own_desc)) return false;
      } else {
        own_desc->value_or_get = v;
      }
      return true;
    }

    if (own_desc->flags & Property::kAccessor) {
      if (!!own_desc->set) {
        // [[CanPut]] returns true.
        Object* set_func = own_desc->set.as<Object>();
        assert(IsCallable(set_func));
        any_ref argv[2];
        argv[0] = this;
        argv[1] = v;
        return !!set_func->Call(2, argv);
      }
    }

    // [[CanPut]] returns false
    return !(do_throw && !ThrowTypeError(c));
  }

  Property* inherited_desc = !!proto() ? proto()->GetProperty(n) : nullptr;
  if (!inherited_desc || (inherited_desc->flags & Property::kWritable)) {
    if (!(flags & kExtensible)) {
      // [[CanPut]] returns false
      return !(do_throw && !ThrowTypeError(c));
    }
    // [[CanPut]] returns true
    Property* new_desc = NewOwnProperty(n);
    new_desc->flags |= Property::kWritable | Property::kEnumerable | Property::kConfigurable;
    if (!!host_data && host_data.is<Array>()) {
      if (!DefineOwnArrayProperty(c, n, v, do_throw, new_desc)) return false;
    } else {
      new_desc->value_or_get = v;
    }
    return true;
  }
  
  if (inherited_desc->flags & Property::kAccessor) {
    if (!!inherited_desc->set) {
      // [[CanPut]] returns true
      Object* set_func = inherited_desc->set.as<Object>();
      assert(IsCallable(set_func));
      any_ref argv[2];
      argv[0] = this;
      argv[1] = v;
      return !!set_func->Call(2, argv);
    }
  }
    
  return !(do_throw && !ThrowTypeError(c));
}

bool Object::Put(Context* c, uint32_t n, any_ref v, bool do_throw) {
  u16string s = uint32_to_u16string(n);
  return Put(c, s, v, do_throw);
}

bool Object::Delete(Context* c, uint32_t n, bool do_throw) {
  u16string s = uint32_to_u16string(n);
  return Delete(c, s, do_throw);
}

Thread* Thread::current_thread_ = nullptr;

Thread::Thread() {
  if (!current_thread_) current_thread_ = this;
}

Thread::~Thread() {
  if (this == current_thread_) current_thread_ = nullptr;
}

void Throw_(any_ref v) {
  Thread* th = Thread::GetCurrent();
  th->Throw(v);
}

void ThrowTypeError_(Context* c) {
  Object* o = Object::Alloc(c->error_proto());
  o->Put(c, "message", "Type error", false);
  Throw(o);
}

void ThrowReferenceError_(Context* c) {
  Object* o = Object::Alloc(c->error_proto());
  o->Put(c, "message", "Reference error", false);
  Throw(o);
}

void ThrowSyntaxError_(Context* c) {
  Object* o = Object::Alloc(c->error_proto());
  o->Put(c, "message", "Syntax error", false);
  Throw(o);
}

any_ref Catch() {
  Thread* th = Thread::GetCurrent();
  return th->Catch();
}

static any_ref get_default_string(Object* obj) {
  any_ref func_val = obj->Get("toString");
  if (!func_val) return nullptr;
  if (func_val.is<Object>()) {
    Object* func_obj = func_val.as<Object>();
    if (IsCallable(func_obj)) {
      any_ref argv = obj;
      return func_obj->Call(1, &argv);
    }
  }
  return obj;
}

static any_ref get_default_number(Object* obj) {
  any_ref func_val = obj->Get("valueOf");
  if (!func_val) return nullptr;
  if (func_val.is<Object>()) {
    Object* func_obj = func_val.as<Object>();
    if (IsCallable(func_obj)) {
      any_ref argv = obj;
      return func_obj->Call(1, &argv);
    }
  }
  return obj;
}

any_ref Object::DefaultValue(Context* c, PreferredType hint) {
  // 8.12.8 [[DefaultValue]] (hint)
  if (hint == Object::kPreferredNone) {
    if (!!host_data && host_data.is<Date>())
      hint = Object::kPreferredString;
    else
      hint = Object::kPreferredNumber;
  }
  if (hint == Object::kPreferredString) {
    any_ref val = get_default_string(this);
    if (!val || !val.is<Object>()) return val;
    val = get_default_number(this);
    if (!val || !val.is<Object>()) return val;
    return ThrowTypeError(c);
  } else {
    assert(hint == Object::kPreferredNumber);
    any_ref val = get_default_number(this);
    if (!val || !val.is<Object>()) return val;
    val = get_default_string(this);
    if (!val || !val.is<Object>()) return val;
    return ThrowTypeError(c);
  }
}

any_ref Object::Call(size_t argc, const any_ref* argv) {
  assert(argc >= 1);
  assert(!!argv[0]);
  Function* fn = host_data.as<Function>();
  if (fn->native_code) {
    return (*fn->native_code)(fn->context, argc, argv);
  } else {
    assert(!!fn->code);
    // 10.4.3 Entering Function Code
    any_ref this_val = argv[0];
    if (!fn->strict) {
      if (this_val.is_undefined() || this_val.is_null()) {
        this_val = fn->context->global_obj();
      } else {
        Object* this_obj = ToObject(fn->context, this_val);
        if (!this_obj) return nullptr;
        this_val = this_obj;
      }
    }
    return AstEvaluator::CallFunction(fn->context, fn->script, fn->scope, fn->code, fn->strict, this_val, argc - 1, argv + 1);
  }
}

any_ref Object::Construct(size_t argc, const any_ref* argv) {
  assert(argc >= 1);
  assert(!argv[0]);
  Function* fn = host_data.as<Function>();
  if (fn->native_code) {
    return (*fn->native_code)(fn->context, argc, argv);
  } else {
    assert(!!fn->code);
    // 13.2.2 [[Construct]]
    any_ref proto_val = Get("prototype");
    Object* this_obj = Object::Alloc(proto_val.as<Object>());
    any_ref rval = AstEvaluator::CallFunction(fn->context, fn->script, fn->scope, fn->code, fn->strict, this_obj, argc - 1, argv + 1);
    if (rval.is<Object>()) return rval;
    return this_obj;
  }
}

Function* Function::Alloc() {
  Function* fn = reinterpret_cast<Function*>(GC_MALLOC(sizeof (Function)));
  fn->tag_ = Function::class_tag;
  return fn;
}

Array* Array::Alloc() {
  Array* fn = reinterpret_cast<Array*>(GC_MALLOC(sizeof (Array)));
  fn->tag_ = Array::class_tag;
  return fn;
}

Date* Date::Alloc() {
  Date* fn = reinterpret_cast<Date*>(GC_MALLOC(sizeof (Date)));
  fn->tag_ = Date::class_tag;
  return fn;
}

any_ref ToPrimitive(Context* c, any_ref v, Object::PreferredType hint) {
  // 9.1 ToPrimitive
  if (!v.is<Object>()) return v;
  Object* o = v.as<Object>();
  return o->DefaultValue(c, hint);
}

bool ToBoolean(any_ref v) {
  // 9.2 ToBoolean
  if (v.is_smi()) {
    return v.smi() != 0;
  } else if (v.is_undefined() || v.is_null()) {
    return false;
  } else if (v.is_bool()) {
    return v.as<bool_data>()->data();
  } else if (v.is_double()) {
    double num = v.as<double_data>()->data();
    return !(num == 0. || std::isnan(num));
  } else if (v.is_u16string()) {
    return v.as<u16string_data>()->length() != 0;
  } else {
    assert(v.is<Object>());
    return true;
  }
}

bool ToNumber(Context* c, any_ref v, double& d) {
  // 9.3 ToNumber
  if (v.is_smi()) {
    d = v.smi();
  } else if (v.is_undefined()) {
    d = std::numeric_limits<double>::quiet_NaN();
  } else if (v.is_null()) {
    d = 0;
  } else if (v.is_bool()) {
    d = v.as_bool();
  } else if (v.is_double()) {
    d = v.as_double();
  } else if (v.is_u16string()) {
    u16string s = v.as_u16string();
    std::string u8s(s.begin(), s.end());
    d = strtod(u8s.c_str(), nullptr);
  } else {
    assert(v.is<Object>());
    v = ToPrimitive(c, v, Object::kPreferredNumber);
    if (!v) return false;
    assert(!v.is<Object>());
    return ToNumber(c, v, d);
  }
  return true;
}

Object* ToObject(Context* c, any_ref v) {
  // 9.9 ToObject
  if (v.is_smi()) {
    Object* o = Object::Alloc(c->number_proto());
    o->host_data = v;
    return o;
  } else if (v.is_undefined() || v.is_null()) {
    ThrowTypeError(c);
    return nullptr;
  } else if (v.is_bool()) {
    Object* o = Object::Alloc(c->boolean_proto());
    o->host_data = v;
    return o;
  } else if (v.is_double()) {
    Object* o = Object::Alloc(c->number_proto());
    o->host_data = v;
    return o;
  } else if (v.is_u16string()) {
    return NewStringObject(c, v.as_u16string());
  } else {
    assert(v.is<Object>());
    return v.as<Object>();
  }
}

u16string ToString(Context* c, any_ref v) {
  assert(!!v);
  if (v.is_smi()) {
    char buf[32];
#ifdef WIN32
    sprintf_s(buf, "%d", v.smi());
#else
    sprintf(buf, "%d", v.smi());
#endif
    return buf;
  } else if (v.is_undefined()) {
    return "undefined";
  } else if (v.is_null()) {
    return "null";
  } else if (v.is_bool()) {
    return v.as<bool_data>()->data() ? "true" : "false";
  } else if (v.is_double()) {
    double dval = v.as_double();
    if (std::isnan(dval)) {
      return "NaN";
    } else if (std::isinf(dval)) {
      if (dval > 0) {
        return "Infinity";
      } else {
        return "-Infinity";
      }
    }
    char buf[32];
#ifdef WIN32
    sprintf_s(buf, "%g", dval);
#else
    sprintf(buf, "%g", dval);
#endif
    return buf;
  } else if (v.is_u16string()) {
    return v.as_u16string();
  } else {
    assert(v.is<Object>());
    v = ToPrimitive(c, v, Object::kPreferredString);
    if (!v) return nullptr;
    return ToString(c, v);
  }
}

bool CheckObjectCoercible(Context* c, any_ref v) {
  // 9.10 CheckObjectCoercible
  if (v.is_smi()) {
    return true;
  } else if (v.is_undefined() || v.is_null()) {
    ThrowReferenceError(c);
    return false;
  }
  return true;
}

u16string TypeOf(any_ref v) {
  // 11.4.3 The typeof Operator
  if (v.is_smi()) {
    return "number";
  } else if (v.is_undefined()) {
    return "undefined";
  } else if (v.is_null()) {
    return "object";
  } else if (v.is_bool()) {
    return "boolean";
  } else if (v.is_double()) {
    return "number";
  } else if (v.is_u16string()) {
    return "string";
  } else {
    assert(v.is<Object>());
    Object* o = v.as<Object>();
    if (IsCallable(o)) {
      return "function";
    } else {
      return "object";
    }
  }
}

bool IsStrictSameValue(any_ref lval, any_ref rval) {
  // 11.9.6 The Strict Equality Comparison Algorithm
  if (lval.is_smi()) {
    if (rval.is_smi()) {
      return lval.smi() == rval.smi();
    } else if (rval.is_double()) {
      double lnum = lval.smi();
      double rnum = rval.as<double_data>()->data();
      return lnum == rnum;
    } else {
      return false;
    }
  } else if (lval.is_double()) {
    if (rval.is_smi()) {
      double lnum = lval.as<double_data>()->data();
      double rnum = rval.smi();
      return lnum == rnum;
    } else if (rval.is_double()) {
      double lnum = lval.as<double_data>()->data();
      double rnum = rval.as<double_data>()->data();
      return lnum == rnum;
    } else {
      return false;
    }
  } else if (lval.is_undefined()) {
    return rval.is_undefined();
  } else if (lval.is_null()) {
    return rval.is_null();
  } else if (lval.is_u16string()) {
    if (rval.is_u16string()) {
      u16string lstr(lval.as<u16string_data>());
      u16string rstr(rval.as<u16string_data>());
      return lstr == rstr;
    } else {
      return false;
    }
  } else if (lval.is_bool()) {
    if (rval.is_bool()) {
      bool l = lval.as<bool_data>()->data();
      bool r = lval.as<bool_data>()->data();
      return l == r;
    } else {
      return false;
    }
  } else {
    if (rval.is<Object>()) {
      Object* lobj = lval.as<Object>();
      Object* robj = rval.as<Object>();
      return lobj == robj;
    } else {
      return false;
    }
  }
}

bool IsAbstractSameValue(Context* c, any_ref xval, any_ref yval, bool& result) {
  // 11.9.3 The Abstract Equality Comparison Algorithm
  if (xval.is<Object>()) {
    if (yval.is<Object>()) {
      result = xval.as<Object>() == yval.as<Object>();
      return true;
    }
    if (yval.is_undefined() || yval.is_null()) {
      result = false;
      return true;
    }
    if (yval.is_bool()) yval = yval.as_bool() ? 1 : 0;
    xval = ToPrimitive(c, xval, Object::kPreferredNone);
    if (!xval) return false;
  }
  if (yval.is<Object>()) {
    yval = ToPrimitive(c, yval, Object::kPreferredNone);
    if (!yval) return false;
  }
  if (xval.is_undefined() || xval.is_null()) {
    result = yval.is_undefined() || yval.is_null();
    return true;
  }
  if (yval.is_undefined() || yval.is_null()) {
    result = false;
    return true;
  }
  if (xval.is_bool()) xval = xval.as_bool() ? 1 : 0;
  if (yval.is_bool()) yval = yval.as_bool() ? 1 : 0;
  if (xval.is_u16string()) {
    if (yval.is_u16string()) {
      u16string xstr = xval.as_u16string();
      u16string ystr = yval.as_u16string();
      result = xstr == ystr;
      return true;
    }
    double d;
    if (!ToNumber(c, xval, d)) return false;
    xval = d;
  }
  if (yval.is_u16string()) {
    double d;
    if (!ToNumber(c, yval, d)) return false;
    yval = d;
  }
  if (xval.is_smi()) {
    if (yval.is_smi()) {
      result = xval.smi() == yval.smi();
      return true;
    } else {
      assert(yval.is_double());
      result = xval.smi() == yval.as_double();
      return true;
    }
  } else {
    assert(xval.is_double());
    if (yval.is_smi()) {
      result = xval.as_double() == yval.smi();
      return true;
    } else {
      assert(yval.is_double());
      result = xval.as_double() == yval.as_double();
      return true;
    }
  }
}

bool IsCallable(Object* o) {
  return !!o->host_data && o->host_data.is<Function>();
}

Script* Script::Alloc(u16string name, Program* program, u16string source) {
  Script* script = gc_malloc_cast<Script>();
  if (!script) return nullptr;
  script->tag_ = Script::class_tag;
  script->name = name;
  script->program_ = program;
  script->source = source;
  script->string_table_.init();
  GC_REGISTER_FINALIZER(script, [](GC_PTR obj, GC_PTR client_data) {
      Script* script = reinterpret_cast<Script*>(obj);
      // std::cout << "delete program: " << script->name << std::endl;
      delete script->program_;
    }, 0, NULL, NULL);
  return script;
}

DeclarativeEnvironment* DeclarativeEnvironment::Alloc(Environment* outer) {
  DeclarativeEnvironment* env = gc_malloc_cast<DeclarativeEnvironment>();
  if (!env) return nullptr;
  env->tag_ = DeclarativeEnvironment::class_tag;
  env->bindings_.init();
  env->outer = outer;
  return env;
}

ObjectEnvironment* ObjectEnvironment::Alloc(Environment* outer) {
  ObjectEnvironment* env = gc_malloc_cast<ObjectEnvironment>();
  if (!env) return nullptr;
  env->tag_ = ObjectEnvironment::class_tag;
  env->outer = outer;
  return env;
}

bool ObjectEnvironment::CreateMutableBinding(Context* c, u16string n, any_ref v, bool can_delete) {
  return bindings_obj->Put(c, n, v, false);
}

extern const char* startup_source;

Context* Context::Alloc(bool ext) {
  Context* c = reinterpret_cast<Context*>(GC_MALLOC(sizeof (Context)));
  c->tag_ = Context::class_tag;
  c->InitStandardBuiltInObjects();
  u16string s = startup_source;
  if (!c->EvalString(s, "[startup]")) {
    assert(false);
  }
  if (ext) {
    c->InitExtendedBuiltInObjects();
  }
  return c;
}

any_ref Context::EvalString(u16string source, u16string name) {
  Parser parser;
  std::u16string u16name(name.data(), name.length());
  Parser::Result<Program> result = parser.ParseProgram(source.data(), source.length(), u16name.c_str());
  if (!result.IsSuccess()) {
    return ThrowSyntaxError(this);
  }
  auto string_map = result.string_map();
  Script* script = Script::Alloc(name, result.ReleaseNode(), source);
  auto& string_table = script->string_table();
  string_table.resize(string_map.size());
  for (auto it = string_map.begin(); it != string_map.end(); ++it) {
    const std::u16string& s = (*it).first;
    int i = (*it).second;
    string_table[i] = u16string_data::alloc(s.data(), s.length());
    // std::cout << i << " -> " << any_ref(string_table[i]) << std::endl;
  }

  return AstEvaluator::EvalScript(this, script);
}

Object* CreateNativeFunction(Context* c, NativeCodeProc proc) {
  Object* o = Object::Alloc(c->function_proto());
  Function* fn = Function::Alloc();
  fn->context = c;
  fn->native_code = proc;
  o->host_data = fn;
  return o;
}

Object* NewStringObject(Context* c, u16string s) {
  Object* o = Object::Alloc(c->string_proto());
  o->host_data = s;
  Property* desc = o->NewOwnProperty("length");
  desc->flags = 0;
  desc->value_or_get = static_cast<double>(s.length());
  return o;
}

Object* NewArrayObject(Context* c, uint32_t n, const any_ref* e) {
  Object* o = Object::Alloc(c->array_proto());
  Array* arr = Array::Alloc();
  arr->length = n;
  o->host_data = arr;
  {
    Property* desc = o->NewOwnProperty("length");
    desc->flags = Property::kWritable | Property::kEnumerable;
    desc->value_or_get = static_cast<double>(n);
  }
  if (e) {
    for (uint32_t i = 0; i < n; i++) {
      u16string s = uint32_to_u16string(i);
      Property* desc = o->NewOwnProperty(s);
      desc->flags = Property::kWritable | Property::kEnumerable | Property::kConfigurable;
      desc->value_or_get = e[i];
    }
  }
  return o;
}

}  // namespace internal
}  // namespace nabla
