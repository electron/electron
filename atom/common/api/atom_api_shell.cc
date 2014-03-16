// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_shell.h"

#include <string>

#include "base/files/file_path.h"
#include "atom/common/platform_util.h"
#include "atom/common/v8/native_type_conversions.h"
#include "url/gurl.h"

#include "atom/common/v8/node_common.h"

namespace atom {

namespace api {

// static
void Shell::ShowItemInFolder(const v8::FunctionCallbackInfo<v8::Value>& args) {
  base::FilePath file_path;
  if (!FromV8Arguments(args, &file_path))
    return node::ThrowTypeError("Bad argument");

  platform_util::ShowItemInFolder(file_path);
}

// static
void Shell::OpenItem(const v8::FunctionCallbackInfo<v8::Value>& args) {
  base::FilePath file_path;
  if (!FromV8Arguments(args, &file_path))
    return node::ThrowTypeError("Bad argument");

  platform_util::OpenItem(file_path);
}

// static
void Shell::OpenExternal(const v8::FunctionCallbackInfo<v8::Value>& args) {
  GURL url;
  if (!FromV8Arguments(args, &url))
    return node::ThrowTypeError("Bad argument");

  platform_util::OpenExternal(url);
}

// static
void Shell::MoveItemToTrash(const v8::FunctionCallbackInfo<v8::Value>& args) {
  base::FilePath file_path;
  if (!FromV8Arguments(args, &file_path))
    return node::ThrowTypeError("Bad argument");

  platform_util::MoveItemToTrash(file_path);
}

// static
void Shell::Beep(const v8::FunctionCallbackInfo<v8::Value>& args) {
  platform_util::Beep();
}

// static
void Shell::Initialize(v8::Handle<v8::Object> target) {
  NODE_SET_METHOD(target, "showItemInFolder", ShowItemInFolder);
  NODE_SET_METHOD(target, "openItem", OpenItem);
  NODE_SET_METHOD(target, "openExternal", OpenExternal);
  NODE_SET_METHOD(target, "moveItemToTrash", MoveItemToTrash);
  NODE_SET_METHOD(target, "beep", Beep);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_shell, atom::api::Shell::Initialize)
