// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "third_party/skia/include/core/SkColor.h"

namespace atom {

// Parse hex color like "#FFF" or "#EFEFEF"
SkColor ParseHexColor(const std::string& name);

}  // namespace atom
