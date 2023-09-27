// Copyright 2023 OpenFin. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_BACKGROUND_THROTTLING_SOURCE
#define ELECTRON_SHELL_BROWSER_BACKGROUND_THROTTLING_SOURCE

namespace electron {

class BackgroundThrottlingSource {
 public:
  virtual ~BackgroundThrottlingSource() = default;
  virtual bool GetBackgroundThrottling() const = 0;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_BACKGROUND_THROTTLING_SOURCE
