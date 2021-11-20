// Copyright (c) 2020 Samuel Maddock
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_WORLD_IDS_H_
#define ELECTRON_SHELL_COMMON_WORLD_IDS_H_

#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"  // nogncheck

namespace electron {

enum WorldIDs : int32_t {
  MAIN_WORLD_ID = 0,

  // Use a high number far away from 0 to not collide with any other world
  // IDs created internally by Chrome.
  ISOLATED_WORLD_ID = 999,

  // Numbers for isolated worlds for extensions are greater than or equal to
  // this number, up to ISOLATED_WORLD_ID_EXTENSIONS_END.
  ISOLATED_WORLD_ID_EXTENSIONS = 1 << 20,

  // Last valid isolated world ID.
  ISOLATED_WORLD_ID_EXTENSIONS_END =
      blink::IsolatedWorldId::kEmbedderWorldIdLimit - 1
};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_WORLD_IDS_H_
