// Copyright (c) 2022 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_THREAD_RESTRICTIONS_H_
#define ELECTRON_SHELL_COMMON_THREAD_RESTRICTIONS_H_

#include "base/threading/thread_restrictions.h"

namespace electron {

class ScopedAllowBlockingForElectron : public base::ScopedAllowBlocking {};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_THREAD_RESTRICTIONS_H_
