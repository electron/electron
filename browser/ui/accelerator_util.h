// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_UI_ACCELERATOR_UTIL_H_
#define BROWSER_UI_ACCELERATOR_UTIL_H_

#include <string>

namespace ui {
class Accelerator;
}

namespace accelerator_util {

// Parse a string as an accelerator.
bool StringToAccelerator(const std::string& description,
                         ui::Accelerator* accelerator);

// Set platform accelerator for the Accelerator.
void SetPlatformAccelerator(ui::Accelerator* accelerator);

}  // namespace accelerator_util

#endif  // BROWSER_UI_ACCELERATOR_UTIL_H_
