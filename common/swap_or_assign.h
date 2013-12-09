// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_SWAP_OR_ASSIGN_H_
#define ATOM_COMMON_SWAP_OR_ASSIGN_H_

#include "base/compiler_specific.h"

namespace internal {

#if defined(OS_WIN)
template<typename T> inline
void SwapOrAssign(T& v1, const T& v2) {
  __if_exists(T::swap) {
    v1.swap(const_cast<T&>(v2));
  }

  __if_not_exists(T::swap) {
    v1 = v2;
  }
}

template<typename T> inline
void SwapOrAssign(T*& v1, T* v2) {
  v1 = v2;
}

inline
void SwapOrAssign(int& v1, int v2) {
  v1 = v2;
}

inline
void SwapOrAssign(bool& v1, bool v2) {
  v1 = v2;
}
#else  // defined(OS_WIN)
// Helper to detect whether value has specified method.
template <typename T>
class HasSwapMethod {
  typedef char one;
  typedef long two;
  template <typename C> static one test(char[sizeof(&C::swap)]) ;
  template <typename C> static two test(...);
 public:
  enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

template<bool B, class T = void>
struct enable_if {};

template<class T>
struct enable_if<true, T> { typedef T type; };

template<typename T> inline
typename enable_if<HasSwapMethod<T>::value>::type SwapOrAssign(
    T& v1, const T& v2) {
  v1.swap(const_cast<T&>(v2));
}

template<typename T> inline
typename enable_if<!HasSwapMethod<T>::value>::type SwapOrAssign(
    T& v1, const T& v2) {
  v1 = v2;
}
#endif  // !defined(OS_WIN)

}  // namespace internal

#endif  // ATOM_COMMON_SWAP_OR_ASSIGN_H_
