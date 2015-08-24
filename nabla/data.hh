/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#pragma once

#ifndef NABLA_DATA_HH_
#define NABLA_DATA_HH_

#include "coll.hh"

#include <cassert>
#include <cinttypes>
#include <cstddef>

namespace nabla {
namespace internal {

void init();
void getmeminfo(size_t& heap_size, size_t& free_bytes);
void gc();

class heap_data;
class bool_data;
template<typename charT> class string_data;
typedef string_data<char16_t> u16string_data;
class Object;

class heap_data {
 public:
  enum tag {
    kTagUndefined = 0,
    kTagNull,
    kTagBool,
    kTagU16String,
    kTagDouble,
    kTagObject,
    kTagContext,
    kTagScript,
    kTagFunction,
    kTagArray,
    kTagDate,
    kTagRegExp,
    kTagDeclarativeEnvironment,
    kTagObjectEnvironment
  };

 protected:
  static const int undefined_index_ = 0;
  static const int nullvalue_index_ = 1;
  static const int true_index_ = 2;
  static const int false_index_ = 3;
  static heap_data value_table_[4];

 public:
  bool is_undefined() const { return tag_ == kTagUndefined; }
  bool is_null() const { return tag_ == kTagNull; }
  bool is_bool() const { return tag_ == kTagBool; }
  bool is_u16string() const { return tag_ == kTagU16String; }
  bool is_double() const { return tag_ == kTagDouble; }
  bool is_object() const { return tag_ == kTagObject; }

  template <typename objectT>
  bool is() const { return tag_ == objectT::class_tag; }

  template <typename objectT>
  objectT* as() {
    assert(is<objectT>());
    return static_cast<objectT*>(this);
  }

 protected:
  tag tag_;

 private:
  friend void init();
};

class undefined_data : public heap_data {
 public:
  static const tag class_tag = kTagUndefined;
  static undefined_data* alloc();
};

class null_data : public heap_data {
 public:
  static const tag class_tag = kTagNull;
  static null_data* alloc();
};

class bool_data : public heap_data {
 public:
  static const tag class_tag = kTagBool;
  static bool_data* alloc(bool b);
  bool data() { return this == &heap_data::value_table_[heap_data::true_index_]; }
};

class string_data_base : public heap_data {
 protected:
  size_t length_;

  static string_data_base* alloc_(size_t n, size_t charsize);
};

template <typename charT>
class string_data : public string_data_base {
 public:
  static const tag class_tag = kTagU16String;

 public:
  static string_data* alloc(size_t n) {
    string_data* ret = reinterpret_cast<string_data*>(alloc_(n, sizeof (charT)));
    ret->tag_ = kTagU16String;
    return ret;
  }

  template <typename charS>
  static string_data* alloc(const charS* s, size_t n) {
    string_data* ret = alloc(n);
    ret->set(0, s, n);
    return ret;
  }

  static string_data* alloc() {
    string_data* ret = alloc(0);
    return ret;
  }

  string_data* concat(const string_data* s) const {
    size_t n1 = length_;
    size_t n2 = s->length_;
    string_data* ret = alloc(n1 + n2);
    ret->set(0, data(), n1);
    ret->set(n1, s->data(), n2);
    return ret;
  }

  template <typename charS>
  bool equals(const string_data<charS>* other) const {
    if (this == other) return true;
    size_t len = length_;
    if (len != other->length_) return false;
    const charS *s = other->data();
    const charT *t = data();
    while (len--) {
      if (*s++ != *t++) return false;
    }
    return true;
  }

  template <typename charS>
  int compare(const string_data<charS>* other) const {
    if (this == other) return 0;
    size_t len = length_ < other->length_ ? length_ : other->length_;
    const charS *s = other->data();
    const charT *t = data();
    while (len--) {
      int diff = (*s++) - (*t++);
      if (diff != 0) return diff;
    }
    return length_ - other->length_;
  }

  uint32_t hash() const {
    uint32_t hash = 0;
    const charT *p = data();
    for (size_t i = 0; i < length_; i++) {
      hash = (hash << 6) + hash + p[i];
    }
    return hash;
  }


  bool empty() const { return !length(); }
  bool to_boolean() const { return !empty(); }

  template <typename charS>
  void set(size_t offset, const charS* s, size_t n) {
    charT *d = &data()[offset];
    while (n--) *d++ = *s++;
  }

  charT* data() {
    uint8_t* p = reinterpret_cast<uint8_t*>(this) + sizeof *this;
    return reinterpret_cast<charT*>(p);
  }

  const charT* data() const {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(this) + sizeof *this;
    return reinterpret_cast<const charT*>(p);
  }

  size_t length() const { return length_; }
};

class double_data : public heap_data {
 public:
  static const tag class_tag = kTagDouble;
  static double_data* alloc(double d);
  double data() { return data_; }

 protected:
  double data_;
};

template <typename charT>
class string {
 public:
  typedef string_data<charT> value_type;

  string() : ptr_(nullptr) {}
  string(const value_type* ptr) : ptr_(ptr) {
    // assert(ptr->tag() == u16string_data::class_tag);
  }
  template<typename charS>
  string(const charS* s, size_t n) : ptr_(value_type::alloc(s, n)) {}
  string(const char* s);
  string(std::nullptr_t) : ptr_(nullptr) {}

  const charT* data() const { return ptr_->data(); }
  const charT* begin() const { return ptr_->data(); }
  const charT* end() const { return ptr_->data() + ptr_->length(); }
  size_t length() const { return ptr_->length(); }
  const value_type* get__() const { return ptr_; }
  const charT& operator [] (size_t n) const { return ptr_->data()[n]; }
  bool operator == (const string& other) const { return ptr_->equals(other.ptr_); }
  bool operator != (const string& other) const { return !(*this == other); }
  string operator + (const string& other) const { return ptr_->concat(other.ptr_); }
  bool is_nil() const { return ptr_ == nullptr; }
  bool operator ! () const { return is_nil(); }
  uint32_t hash() const { return ptr_->hash(); }
 private:
  const value_type* ptr_;
};

typedef string<char16_t> u16string;

#if __SIZEOF_POINTER__ > 32
#define JS_SMI_SHIFT 32
#else
#define JS_SMI_SHIFT 1
#endif

class any_ref {
 public:
  any_ref() { data_ = 0; }
  any_ref(std::nullptr_t) : any_ref() {}
  any_ref(bool b);
  any_ref(int n) { data_ = (static_cast<intptr_t>(n) << JS_SMI_SHIFT) | 1; }
  any_ref(heap_data* o) { data_ = reinterpret_cast<intptr_t>(o); }
  any_ref(const u16string s) : any_ref((u16string_data*)s.get__()) {}
  template <typename charS>
  any_ref(const charS* s, size_t n) : any_ref(u16string_data::alloc(s, n)) {}
  any_ref(double d) : any_ref(double_data::alloc(d)) {}
  any_ref(const char* s);

  int smi() const {
    assert(is_smi());
    return static_cast<int>(data_ >> JS_SMI_SHIFT);
  }

  heap_data* get() const {
    assert(is_heap_data());
    return reinterpret_cast<heap_data*>(data_);
  }

  bool is_nil() const { return is_heap_data() && !get(); }
  bool is_smi() const { return (data_ & 1) != 0; }
  bool is_heap_data() const { return !is_smi(); }

  template <typename objectT>
  bool is() const { return !is_nil() && is_heap_data() && get()->is<objectT>(); }
  template <typename objectT>
  bool is_unsafe() const { return get()->is<objectT>(); }
  template <typename objectT>
  objectT* as() const { assert(!is_nil()); return get()->as<objectT>(); }
  
  bool is_undefined() const { return is_heap_data() && get()->is_undefined(); }
  bool is_null() const { return is_heap_data() && get()->is_null(); }
  bool is_u16string() const { return is_heap_data() && get()->is_u16string(); }
  bool is_bool() const { return is_heap_data() && get()->is_bool(); }
  bool is_double() const { return is_heap_data() && get()->is_double(); }
  u16string as_u16string() const {
    assert(is_u16string());
    return static_cast<u16string_data*>(get());
  }
  double as_bool() const {
    assert(is_bool());
    return static_cast<bool_data*>(get())->data();
  }
  double as_double() const {
    assert(is_double());
    return static_cast<double_data*>(get())->data();
  }
  bool operator ! () const { return is_nil(); }

 private:
  intptr_t data_;
};

typedef vector<any_ref> any_vector;
typedef map<u16string_data*, any_ref> any_map;

}  // namespace internal
}  // namespace nabla

#endif  // NABLA_DATA_HH_
