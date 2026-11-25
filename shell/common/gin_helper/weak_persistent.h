// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_WEAK_PERSISTENT_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_WEAK_PERSISTENT_H_

#include <type_traits>

#include "v8/include/cppgc/persistent.h"
#include "v8/include/cppgc/source-location.h"

namespace gin_helper {

// Based on WrapWeakPersistent() in
// third_party/blink/renderer/platform/heap/persistent.h.
//
// Use this to bind a garbage-collected receiver weakly in base::Bind*():
//
//   base::BindOnce(&Foo::OnDone, gin_helper::WrapWeakPersistent(this));
//
// The callback becomes a no-op once |this| has been collected. Only methods
// that return void can be bound to a weak receiver. Use a gin::WeakCellFactory
// instead when callbacks must be invalidated explicitly before GC.
template <typename T>
cppgc::WeakPersistent<T> WrapWeakPersistent(
    T* value,
    const cppgc::SourceLocation& loc = cppgc::SourceLocation()) {
  return cppgc::WeakPersistent<T>(value, loc);
}

}  // namespace gin_helper

// These mirror the specializations in
// third_party/blink/renderer/platform/heap/persistent.h, which is internal to
// Blink and never included alongside this header.
namespace base {

template <typename T>
struct IsWeakReceiver;

template <typename T>
struct IsWeakReceiver<cppgc::WeakPersistent<T>> : std::true_type {};

template <typename>
struct MaybeValidTraits;

template <typename T>
struct MaybeValidTraits<cppgc::WeakPersistent<T>> {
  static constexpr bool MaybeValid(const cppgc::WeakPersistent<T>& p) {
    // Not necessarily called on the owning thread, so the only possible
    // implementation is to assume the object has not been collected.
    return true;
  }
};

}  // namespace base

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_WEAK_PERSISTENT_H_
