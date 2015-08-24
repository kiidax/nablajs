/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#pragma once

#ifndef NABLA_COLL_HH_
#define NABLA_COLL_HH_

#include <cstddef>
#include <cstring>
#include <cassert>

namespace nabla {
namespace internal {

void* gc_malloc(size_t n);
void* gc_realloc(void* p, size_t n);
void* gc_malloc_atomic(size_t n);
template <typename T>
inline T* gc_malloc_cast() {
  return reinterpret_cast<T*>(gc_malloc(sizeof (T)));
}
template <typename T>
inline T* gc_realloc_array_cast(T* p, size_t n) {
  return reinterpret_cast<T*>(gc_realloc(reinterpret_cast<void*>(p), n * sizeof (T)));
}
template <typename T>
inline T* gc_malloc_cast_atomic() {
  return reinterpret_cast<T*>(gc_malloc_atomic(sizeof (T)));
}

template <typename T, typename S>
struct pair {
 public:
  T first;
  S second;
};

template <typename T>
class list {
 protected:
  struct node {
    node* prev;
    node* next;
  };

  struct node_value : public node {
    T v;
  };

 public:
  class iterator {
   public:
    typedef T value_type;

    iterator& operator ++ () { cur_ = cur_->next; return *this; }
    iterator& operator -- () { cur_ = cur_->prev; return *this; }
    value_type& operator * () { return reinterpret_cast<node_value*>(cur_)->v; }
    bool operator == (const iterator& o) const { return cur_ == o.cur_; }
    bool operator != (const iterator& o) const { return cur_ != o.cur_; }

   private:
    iterator(node* cur) : cur_(cur) {}

    node* cur_;

    friend class list;
  };

  class const_iterator {
   public:
    typedef T value_type;

    const_iterator& operator ++ () { cur_ = cur_->next; return *this; }
    const_iterator& operator -- () { cur_ = cur_->prev; return *this; }
    const value_type& operator * () { return reinterpret_cast<const node_value*>(cur_)->v; }
    bool operator == (const const_iterator& o) const { return cur_ == o.cur_; }
    bool operator != (const const_iterator& o) const { return cur_ != o.cur_; }

   private:
    const_iterator(const node* cur) : cur_(cur) {}

    const node* cur_;

    friend class list;
  };

 public:
  void init() { list_.next = &list_; list_.prev = &list_; }
  void push_back(const T& v) {
    node_value* nv = gc_malloc_cast<node_value>();
    nv->next = &list_;
    nv->prev = list_.prev;
    list_.prev->next = nv;
    list_.prev = nv;
    nv->v = v;
  }

  T& pop_back() {
    assert(list_.prev != &list_);
    node_value* nv = list_.prev;
    nv->prev->next = &list_;
    list_.prev = nv->prev;
    return nv;
  }

  iterator begin() { return iterator(list_.next); }
  iterator end() { return iterator(&list_); }
  const_iterator begin() const { return const_iterator(list_.next); }
  const_iterator end() const { return const_iterator(&list_); }
  void erase(iterator position) {
    position.cur_->prev->next = position.cur_->next;
    position.cur_->next->prev = position.cur_->prev;
  }

 protected:
  node list_;
};

template <typename T>
class vector {
 public:
  typedef T* iterator;
  typedef const T* const_iterator;
  void init() { data_ = nullptr; size_ = 0; capacity_ = 0; }
  T& operator [] (size_t i) { return data_[i]; }
  const T& operator [] (size_t i) const { return data_[i]; }
  size_t size() const { return size_; }
  void resize(size_t n) { reserve(n); size_ = n; }
  void reserve(size_t n) {
    if (n >= capacity_) {
      data_ = gc_realloc_array_cast<T>(data_, (n + 63) & ~63);
      capacity_ = n;
    }
  }
  iterator begin() { return data_; }
  iterator end() { return data_ + size_; }
  const_iterator begin() const { return data_; }
  const_iterator end() const { return data_ + size_; }
  iterator erase(iterator first, iterator last) {
    iterator ret = first;
    while (last != end()) {
      *first = *last;
      ++first;
      ++last;
    }
    size_ = first - begin();
    return ret;
  }
  iterator erase(iterator position) {
    return erase(position, position + 1);
  }
  void push_back(const T& val) {
    reserve(size_ + 1);
    (*this)[size_++] = val;
  }
  T& pop_back() {
    return (*this)[--size_];
  }
  T* data() { return data_; }
 private:
  T* data_;
  size_t size_;
  size_t capacity_;
};

template <typename K, typename T>
class map {
 public:
  typedef K key_type;
  typedef T mapped_type;
  typedef pair<key_type, mapped_type> value_type;
  typedef list<value_type> list_type;
  typedef typename list_type::iterator iterator;
  typedef typename list_type::const_iterator const_iterator;

 public:
  void init() { list_.init(); }

  mapped_type& operator [] (const key_type& k) {
    iterator it = find(k);
    if (it == end()) {
      value_type v;
      v.first = k;
      list_.push_back(v);
      it = end();
      --it;
    }
    return (*it).second;
  }

  void erase(iterator first, iterator last) {
    list_.erase(first, last);
  }

  void erase(iterator position) {
    list_.erase(position);
  }

  iterator find(const key_type& k) {
    auto it = list_.begin();
    while (it != list_.end()) {
      value_type& v = *it;
      if (v.first == k) return it;
      ++it;
    }
    return it;
  }

  const_iterator find(const key_type& k) const {
    auto it = list_.begin();
    while (it != list_.end()) {
      const value_type& v = *it;
      if (v.first == k) return it;
      ++it;
    }
    return it;
  }

  iterator begin() { return list_.begin(); }
  iterator end() { return list_.end(); }
  const_iterator begin() const { return list_.begin(); }
  const_iterator end() const { return list_.end(); }

 private:
  list_type list_;
};

#define HASH_SIZE 128
  
template <typename K, typename T>
class hash_map {
 public:
  typedef K key_type;
  typedef T mapped_type;
  typedef pair<key_type, mapped_type> value_type;
  typedef list<value_type> list_type;

  class node_type {
   public:
    size_t hash;
    value_type value;
  };

  class iterator {
   public:
    iterator& operator ++ () {
      node++;
      if (node >= m->table[index] + m->table_len[index]) {
        while (++index < HASH_SIZE) {
          if (m->table[index]) {
            node = m->table[index];
            return *this;
          }
        }
        node = nullptr;
        return *this;
      }
      return *this;
    }
    value_type& operator * () { return node->value; }
    bool operator == (const iterator& other) const { return node == other.node; }
    bool operator != (const iterator& other) const { return !((*this) == other); }

    hash_map* m;
    int index;
    node_type* node;
  };

  class const_iterator {
   public:
    const_iterator& operator ++ () {
      node++;
      if (node >= m->table[index] + m->table_len[index]) {
        while (++index < HASH_SIZE) {
          if (m->table[index]) {
            node = m->table[index];
            return *this;
          }
        }
        node = nullptr;
        return *this;
      }
      return *this;
    }
    const value_type& operator * () const { return node->value; }
    bool operator == (const const_iterator& other) const { return node == other.node; }
    bool operator != (const const_iterator& other) const { return !((*this) == other); }

    const hash_map *m;
    int index;
    const node_type* node;
  };

 public:
  void init() {
    memset(table, 0, sizeof table);
    memset(table_len, 0, sizeof table_len);
  }

  mapped_type& operator [] (const key_type& k) {
    iterator it = find(k);
    if (it == end()) {
      size_t hash = k.hash();
      int i = hash & (HASH_SIZE - 1);
      int& len = table_len[i];
      node_type* n = gc_realloc_array_cast<node_type>(table[i], len + 1);
      n[len].hash = hash;
      n[len].value.first = k;
      table[i] = n;
      return n[len++].value.second;
    }
    return (*it).second;
  }

  iterator find(const key_type& k) {
    size_t hash = k.hash();
    int i = hash & (HASH_SIZE - 1);
    node_type* node = table[i];
    
    while (node < table[i] + table_len[i]) {
      if (node->hash == hash && k == node->value.first) {
        return { this, i, node };
      }
      node++;
    }
    return end();
  }

  const_iterator find(const key_type& k) const {
    size_t hash = k.hash();
    int i = hash & (HASH_SIZE - 1);
    const node_type* node = table[i];
    
    while (node < table[i] + table_len[i]) {
      if (node->hash == hash && k == node->value.first) {
        return { this, i, node };
      }
      node++;
    }
    return end();
  }

  void erase(iterator position) {
    node_type* node = table[position.index];
    size_t len = table_len[position.index];
    node_type* end = node + len - 1;
    node_type* pos = position.node;
    while (pos < end) {
      *pos = *(pos + 1);
      pos++;
    }
    table_len[position.index]--;
  }

  iterator begin() {
    for (int i = 0; i < HASH_SIZE; i++) {
      if (table[i]) return { this, i, table[i] };
    }
    return end();
  }
  iterator end() { return { this, HASH_SIZE, nullptr }; }
  const_iterator begin() const {
    for (int i = 0; i < HASH_SIZE; i++) {
      if (table[i]) return { this, i, table[i] };
    }
    return end();
  }
  const_iterator end() const { return { this, HASH_SIZE, nullptr }; }

 private:
  node_type* table[HASH_SIZE];
  int table_len[HASH_SIZE];
};

}  // namespace internal
}  // namespace nabla

#endif  // NABLA_COLL_HH_
