// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "atom/common/platform_util.h"
#include "base/files/file_path.h"
#include "native_mate/object_template_builder.h"
#include "url/gurl.h"

#include "atom/common/v8/node_common.h"

namespace mate {

template<>
struct Converter<GURL> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     GURL* out) {
    std::string url;
    if (Converter<std::string>::FromV8(isolate, val, &url)) {
      *out = GURL(url);
      return true;
    } else {
      return false;
    }
  }
};

template<>
struct Converter<base::FilePath> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     base::FilePath* out) {
    std::string path;
    if (Converter<std::string>::FromV8(isolate, val, &path)) {
      *out = base::FilePath::FromUTF8Unsafe(path);
      return true;
    } else {
      return false;
    }
  }
};

}  // namespace mate

namespace {

void Initialize(v8::Handle<v8::Object> exports) {
  mate::ObjectTemplateBuilder builder(v8::Isolate::GetCurrent());
  builder.SetMethod("showItemInFolder", &platform_util::ShowItemInFolder)
         .SetMethod("openItem", &platform_util::OpenItem)
         .SetMethod("openExternal", &platform_util::OpenExternal)
         .SetMethod("moveItemToTrash", &platform_util::MoveItemToTrash)
         .SetMethod("beep", &platform_util::Beep);
  exports->SetPrototype(builder.Build()->NewInstance());
}

}  // namespace

NODE_MODULE(atom_common_shell, Initialize)
