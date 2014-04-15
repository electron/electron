// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NATIVE_MATE_TEMPLATE_UTIL_H_
#define NATIVE_MATE_TEMPLATE_UTIL_H_

#include <cstddef>  // For size_t.

#include "build/build_config.h"

namespace mate {

// template definitions from tr1

template<class T, T v>
struct integral_constant {
  static const T value = v;
  typedef T value_type;
  typedef integral_constant<T, v> type;
};

template <class T, T v> const T integral_constant<T, v>::value;

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template <class T> struct is_pointer : false_type {};
template <class T> struct is_pointer<T*> : true_type {};

// Member function pointer detection up to four params. Add more as needed
// below. This is built-in to C++ 11, and we can remove this when we switch.
template<typename T>
struct is_member_function_pointer : false_type {};

template <typename R, typename Z>
struct is_member_function_pointer<R(Z::*)()> : true_type {};
template <typename R, typename Z>
struct is_member_function_pointer<R(Z::*)() const> : true_type {};

template <typename R, typename Z, typename A>
struct is_member_function_pointer<R(Z::*)(A)> : true_type {};
template <typename R, typename Z, typename A>
struct is_member_function_pointer<R(Z::*)(A) const> : true_type {};

template <typename R, typename Z, typename A, typename B>
struct is_member_function_pointer<R(Z::*)(A, B)> : true_type {};
template <typename R, typename Z, typename A, typename B>
struct is_member_function_pointer<R(Z::*)(A, B) const> : true_type {};

template <typename R, typename Z, typename A, typename B, typename C>
struct is_member_function_pointer<R(Z::*)(A, B, C)> : true_type {};
template <typename R, typename Z, typename A, typename B, typename C>
struct is_member_function_pointer<R(Z::*)(A, B, C) const> : true_type {};

template <typename R, typename Z, typename A, typename B, typename C,
          typename D>
struct is_member_function_pointer<R(Z::*)(A, B, C, D)> : true_type {};
template <typename R, typename Z, typename A, typename B, typename C,
          typename D>
struct is_member_function_pointer<R(Z::*)(A, B, C, D) const> : true_type {};


template <class T, class U> struct is_same : public false_type {};
template <class T> struct is_same<T,T> : true_type {};

template<class> struct is_array : public false_type {};
template<class T, size_t n> struct is_array<T[n]> : public true_type {};
template<class T> struct is_array<T[]> : public true_type {};

template <class T> struct is_non_const_reference : false_type {};
template <class T> struct is_non_const_reference<T&> : true_type {};
template <class T> struct is_non_const_reference<const T&> : false_type {};

template <class T> struct is_const : false_type {};
template <class T> struct is_const<const T> : true_type {};

template <class T> struct is_void : false_type {};
template <> struct is_void<void> : true_type {};

namespace internal {

// Types YesType and NoType are guaranteed such that sizeof(YesType) <
// sizeof(NoType).
typedef char YesType;

struct NoType {
  YesType dummy[2];
};

// This class is an implementation detail for is_convertible, and you
// don't need to know how it works to use is_convertible. For those
// who care: we declare two different functions, one whose argument is
// of type To and one with a variadic argument list. We give them
// return types of different size, so we can use sizeof to trick the
// compiler into telling us which function it would have chosen if we
// had called it with an argument of type From.  See Alexandrescu's
// _Modern C++ Design_ for more details on this sort of trick.

struct ConvertHelper {
  template <typename To>
  static YesType Test(To);

  template <typename To>
  static NoType Test(...);

  template <typename From>
  static From& Create();
};

// Used to determine if a type is a struct/union/class. Inspired by Boost's
// is_class type_trait implementation.
struct IsClassHelper {
  template <typename C>
  static YesType Test(void(C::*)(void));

  template <typename C>
  static NoType Test(...);
};

}  // namespace internal

// Inherits from true_type if From is convertible to To, false_type otherwise.
//
// Note that if the type is convertible, this will be a true_type REGARDLESS
// of whether or not the conversion would emit a warning.
template <typename From, typename To>
struct is_convertible
    : integral_constant<bool,
                        sizeof(internal::ConvertHelper::Test<To>(
                                   internal::ConvertHelper::Create<From>())) ==
                        sizeof(internal::YesType)> {
};

template <typename T>
struct is_class
    : integral_constant<bool,
                        sizeof(internal::IsClassHelper::Test<T>(0)) ==
                            sizeof(internal::YesType)> {
};

template<bool B, class T = void>
struct enable_if {};

template<class T>
struct enable_if<true, T> { typedef T type; };

}  // namespace mate

#endif  // NATIVE_MATE_TEMPLATE_UTIL_H_
