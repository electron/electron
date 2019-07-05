// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UNRESPONSIVE_SUPPRESSOR_H_
#define SHELL_BROWSER_UNRESPONSIVE_SUPPRESSOR_H_

#include "base/macros.h"

namespace electron {

bool IsUnresponsiveEventSuppressed();

class UnresponsiveSuppressor {
 public:
  UnresponsiveSuppressor();
  ~UnresponsiveSuppressor();

 private:
  DISALLOW_COPY_AND_ASSIGN(UnresponsiveSuppressor);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UNRESPONSIVE_SUPPRESSOR_H_
