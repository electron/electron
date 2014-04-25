// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/common/platform_util.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace {

void Initialize(v8::Handle<v8::Object> exports) {
  mate::Dictionary dict(v8::Isolate::GetCurrent(), exports);
  dict.SetMethod("showItemInFolder", &platform_util::ShowItemInFolder);
  dict.SetMethod("openItem", &platform_util::OpenItem);
  dict.SetMethod("openExternal", &platform_util::OpenExternal);
  dict.SetMethod("moveItemToTrash", &platform_util::MoveItemToTrash);
  dict.SetMethod("beep", &platform_util::Beep);
}

}  // namespace

NODE_MODULE(atom_common_shell, Initialize)
