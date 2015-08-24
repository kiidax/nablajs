/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#pragma once

#ifndef NABLA_CONTEXT_HH_
#define NABLA_CONTEXT_HH_

#include "coll.hh"
#include "data.hh"
#include "ast.hh"

namespace nabla {
namespace internal {

class Context;
class Script;
class Environment;
class Object;

struct Property {
 public:
  enum {
    kNone = 0x0,
    kAccessor = 0x1,
    kWritable = 0x2,
    kEnumerable = 0x4,
    kConfigurable = 0x8
  };

  any_ref value_or_get;
  any_ref set;
  int flags;

 private:
};

typedef hash_map<u16string, Property> ObjectPropertyMap;

class Object : public heap_data {
 public:
  enum PreferredType {
    kPreferredNone, kPreferredString, kPreferredNumber
  };
  enum {
    kExtensible = 0x1
  };

 public:
  static const tag class_tag = kTagObject;
  static Object* Alloc(Object* prototype);

  Object* proto() { return proto_; }
  ObjectPropertyMap& own_props() { return own_props_; }
  
  Property* NewOwnProperty(u16string n) {
    return &own_props_[n];
  }
  Property* GetOwnProperty(u16string n) {
    auto it = own_props_.find(n);
    if (it == own_props_.end()) return nullptr;
    return &(*it).second;
  }
  Property* GetProperty(u16string n);
  Property* GetProperty(uint32_t n);
  any_ref Get(u16string n);
  any_ref Get(uint32_t n);
  any_ref Get(const Property* desc);
  bool Put(Context* c, u16string n, any_ref v, bool do_throw);
  bool Put(Context* c, uint32_t n, any_ref v, bool do_throw);
  bool Delete(Context* c, u16string n, bool do_throw);
  bool Delete(Context* c, uint32_t n, bool do_throw);
  bool Delete(u16string n) { return Delete(nullptr, n, false); }
  any_ref DefaultValue(Context* cx, PreferredType hint);
  bool DefineOwnArrayProperty(Context* c, u16string n, any_ref v, bool do_throw, Property* own_desc);
  void DefineOwnDataPropertyNoCheck(u16string n, any_ref v, int flags);
  
  any_ref Call(size_t argc, const any_ref* argv);
  any_ref Construct(size_t argc, const any_ref* argv);

 public:
  any_ref host_data;

 private:
  Object* proto_;
  ObjectPropertyMap own_props_;
  int flags;
};

typedef any_ref (*NativeCodeProc)(Context* c, size_t argc, const any_ref* argv);

class Function : public heap_data {
 public:
  static const tag class_tag = kTagFunction;
  static Function* Alloc();

 public:
  Script* script;
  Context* context;
  Environment* scope;
  NativeCodeProc native_code;
  FunctionNode* code;
  bool strict;
};

class Array : public heap_data {
 public:
  static const tag class_tag = kTagArray;
  static Array* Alloc();

 public:
  uint32_t length;
};

class Date : public heap_data {
 public:
  static const tag class_tag = kTagDate;
  static Date* Alloc();

 public:
  uint64_t value;
};

class Script : heap_data {
 public:
  static const tag class_tag = kTagScript;
  static Script* Alloc(u16string name, Program* program, u16string source);
  Program *program() { return program_; }
  vector<u16string_data*>& string_table() { return string_table_; }

 private:
  u16string name;
  Program *program_;
  u16string source;
  vector<u16string_data*> string_table_;
};

class Binding {
 public:
  bool immutable;
  bool deletable;
  any_ref value;
};

class Environment : public heap_data {
 public:
  Environment* outer;
};
  
class DeclarativeEnvironment : public Environment {
 public:
  static const tag class_tag = kTagDeclarativeEnvironment;
  static DeclarativeEnvironment* Alloc(Environment* outer);
  Binding* GetBinding(u16string n) {
    auto it = bindings_.find(n);
    if (it == bindings_.end()) return nullptr;
    return &(*it).second;
  }
  any_ref GetBindingValue(u16string n) {
    Binding* b = GetBinding(n);
    return b ? b->value : nullptr;
  }
  bool SetMutableBindingIfFound(Context* c, u16string n, any_ref v, bool strict, bool& found);
  void CreateBinding(u16string n, any_ref v, bool immutable, bool deletable) {
    if (!GetBinding(n)) {
      bindings_[n].immutable = immutable;
      bindings_[n].deletable = deletable;
      bindings_[n].value = v;
    }
  }
  bool DeleteBinding(u16string n) {
    auto it = bindings_.find(n);
    if (it == bindings_.end()) return true;
    Binding* b = &(*it).second;
    if (!b->deletable) return false;
    bindings_.erase(it);
    return true;
  }
  
 private:
  map<u16string, Binding> bindings_;
};

class ObjectEnvironment : public Environment {
 public:
  static const tag class_tag = kTagObjectEnvironment;
  static ObjectEnvironment* Alloc(Environment* outer);
  Property* GetBinding(u16string n) {
    return bindings_obj->GetProperty(n);
  }
  bool SetMutableBinding(Context* c, u16string n, any_ref v, bool strict) {
    return bindings_obj->Put(c, n, v, strict);
  }
  bool SetMutableBindingIfFound(Context* c, u16string n, any_ref v, bool strict, bool& found) {
    Property* prop = bindings_obj->GetProperty(n);
    if (!!prop) {
      found = true;
      return SetMutableBinding(c, n, v, strict);
    } else {
      found = false;
      return true;
    }
  }
  bool CreateMutableBinding(Context* c, u16string n, any_ref v, bool can_delete);
  bool DeleteBinding(u16string n) {
    return bindings_obj->Delete(n);
  }
  
  Object* bindings_obj;
  bool provide_this;
};

class Context : public heap_data {
 public:
  static const tag class_tag = kTagContext;
  static Context* Alloc(bool ext);

  any_ref EvalString(u16string source, u16string name);

  Object* global_obj() const { return global_obj_; }

  // Prototype objects
  Object* object_proto() const { return object_proto_; }
  Object* function_proto() const { return function_proto_; }
  Object* array_proto() const { return array_proto_; }
  Object* string_proto() const { return string_proto_; }
  Object* boolean_proto() const { return boolean_proto_; }
  Object* number_proto() const { return number_proto_; }
  Object* date_proto() const { return date_proto_; }
  Object* regexp_proto() const { return regexp_proto_; }
  Object* error_proto() const { return error_proto_; }

private:
  void InitStandardBuiltInObjects();
  void InitExtendedBuiltInObjects();

  Object* global_obj_;

  Object* object_proto_;
  Object* function_proto_;
  Object* array_proto_;
  Object* string_proto_;
  Object* boolean_proto_;
  Object* number_proto_;
  Object* date_proto_;
  Object* regexp_proto_;
  Object* error_proto_;
};

class Thread {
 public:
  static Thread* GetCurrent() {
    assert(current_thread_);
    return current_thread_;
  }
  static Thread* current_thread_;

 public:
  Thread();
  ~Thread();
  void Throw(any_ref v) {
    assert(!!v);
    exception_val = v;
  }
  any_ref Catch() {
    any_ref v = exception_val;
    exception_val = nullptr;
    return v;
  }

 private:
  any_ref exception_val;
};

any_ref ToPrimitive(Context* c, any_ref v, Object::PreferredType hint = Object::kPreferredNone);
bool ToNumber(Context* c, any_ref v, double& d);
template <typename intT>
inline bool ToInteger(Context* c, any_ref v, intT& n) {
  double d;
  if (!ToNumber(c, v, d)) return false;
  n = static_cast<intT>(d);
  return true;
}
bool ToBoolean(any_ref v);
u16string ToString(Context* c, any_ref v);
Object* ToObject(Context* c, any_ref v);
u16string TypeOf(any_ref v);
bool IsCallable(Object* o);
bool CheckObjectCoercible(Context* c, any_ref v);
bool IsStrictSameValue(any_ref lval, any_ref rval);
bool IsAbstractSameValue(Context* c, any_ref lval, any_ref rval, bool& result);
void Throw_(any_ref v);
inline nullptr_t Throw(any_ref v) { Throw_(v); return nullptr; }
void ThrowTypeError_(Context* c);
inline nullptr_t ThrowTypeError(Context* c) { ThrowTypeError_(c); return nullptr; }
void ThrowSyntaxError_(Context* c);
inline nullptr_t ThrowSyntaxError(Context* c) { ThrowSyntaxError_(c); return nullptr; }
void ThrowReferenceError_(Context* c);
inline nullptr_t ThrowReferenceError(Context* c) { ThrowReferenceError_(c); return nullptr; }
any_ref Catch();
Object* CreateNativeFunction(Context* c, NativeCodeProc proc);
Object* NewStringObject(Context* c, u16string s);
Object* NewArrayObject(Context* c, uint32_t n = 0, const any_ref* e = nullptr);
Object* NewRegExpObject(Context* c, u16string pattern_str, u16string flags_str);

inline bool Object::Delete(Context* c, u16string n, bool do_throw) {
  auto it = own_props_.find(n);
  if (it == own_props_.end()) return true;
  if (!((*it).second.flags & Property::kConfigurable)) {
    if (do_throw) return ThrowTypeError(c);
    return true;
  }
  own_props_.erase(it);
  return true;
}

inline bool DeclarativeEnvironment::SetMutableBindingIfFound(Context* c, u16string n, any_ref v, bool strict, bool& found) {
  Binding* b = GetBinding(n);
  if (b) {
    if (b->immutable) {
      if (strict) return ThrowTypeError(c);
    } else {
      bindings_[n].value = v;
    }
    found = true;
    return true;
  } else {
    found = false;
    return true;
  }
}

}  // namespace internal
}  // namespace nabla

#endif  // NABLA_CONTEXT_HH_
