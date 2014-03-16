// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/ui/accelerator_util.h"

#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/platform_accelerator_gtk.h"

namespace accelerator_util {

void SetPlatformAccelerator(ui::Accelerator* accelerator) {
  scoped_ptr<ui::PlatformAccelerator> platform_accelerator(
      new ui::PlatformAcceleratorGtk(
          GetGdkKeyCodeForAccelerator(*accelerator),
          GetGdkModifierForAccelerator(*accelerator)));
  accelerator->set_platform_accelerator(platform_accelerator.Pass());
}

}  // namespace accelerator_util
