// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UNRESPONSIVE_SUPPRESSOR_H_
#define SHELL_BROWSER_UNRESPONSIVE_SUPPRESSOR_H_

namespace electron {

bool IsUnresponsiveEventSuppressed();

class UnresponsiveSuppressor {
 public:
  UnresponsiveSuppressor();
  ~UnresponsiveSuppressor();

  // disable copy
  UnresponsiveSuppressor(const UnresponsiveSuppressor&) = delete;
  UnresponsiveSuppressor& operator=(const UnresponsiveSuppressor&) = delete;
};

}  // namespace electron

#endif  // SHELL_BROWSER_UNRESPONSIVE_SUPPRESSOR_H_
