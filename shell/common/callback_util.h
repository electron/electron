// Copyright (c) Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_CALLBACK_UTIL_H_
#define ELECTRON_SHELL_COMMON_CALLBACK_UTIL_H_

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"

namespace electron {

namespace internal {
template <typename... Args>
class OnceCallbackHolder {
 public:
  explicit OnceCallbackHolder(base::OnceCallback<void(Args...)> cb)
      : cb_(std::move(cb)) {}

  void Run(Args... args) {
    if (cb_)
      std::move(cb_).Run(std::forward<Args>(args)...);
  }

 private:
  base::OnceCallback<void(Args...)> cb_;
};
}  // namespace internal

template <typename... Args>
base::RepeatingCallback<void(Args...)> AdaptCallbackForRepeating(
    base::OnceCallback<void(Args...)> cb) {
  using Holder = internal::OnceCallbackHolder<Args...>;
  return base::BindRepeating(&Holder::Run,
                             std::make_unique<Holder>(std::move(cb)));
}

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_CALLBACK_UTIL_H_
