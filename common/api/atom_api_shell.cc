// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/atom_api_shell.h"

#include <string>

#include "base/files/file_path.h"
#include "common/platform_util.h"
#include "googleurl/src/gurl.h"
#include "vendor/node/src/node.h"

namespace atom {

namespace api {

namespace {

bool V8ValueToFilePath(v8::Handle<v8::Value> value, base::FilePath* path) {
  if (!value->IsString())
    return false;

  std::string file_path_string(*v8::String::Utf8Value(value));
  base::FilePath file_path = base::FilePath::FromUTF8Unsafe(file_path_string);

  if (file_path.empty())
    return false;

  *path = file_path;
  return true;
}

}  // namespace

// static
v8::Handle<v8::Value> Shell::ShowItemInFolder(const v8::Arguments &args) {
  v8::HandleScope scope;

  base::FilePath file_path;
  if (!V8ValueToFilePath(args[0], &file_path))
    return node::ThrowTypeError("Bad argument");

  platform_util::ShowItemInFolder(file_path);
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Shell::OpenItem(const v8::Arguments &args) {
  v8::HandleScope scope;

  base::FilePath file_path;
  if (!V8ValueToFilePath(args[0], &file_path))
    return node::ThrowTypeError("Bad argument");

  platform_util::OpenItem(file_path);
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Shell::OpenExternal(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsString())
    return node::ThrowTypeError("Bad argument");

  platform_util::OpenExternal(GURL(*v8::String::Utf8Value(args[0])));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Shell::MoveItemToTrash(const v8::Arguments &args) {
  v8::HandleScope scope;

  base::FilePath file_path;
  if (!V8ValueToFilePath(args[0], &file_path))
    return node::ThrowTypeError("Bad argument");

  platform_util::MoveItemToTrash(file_path);
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Shell::Beep(const v8::Arguments &args) {
  platform_util::Beep();
  return v8::Undefined();
}

// static
void Shell::Initialize(v8::Handle<v8::Object> target) {
  node::SetMethod(target, "showItemInFolder", ShowItemInFolder);
  node::SetMethod(target, "openItem", OpenItem);
  node::SetMethod(target, "openExternal", OpenExternal);
  node::SetMethod(target, "moveItemToTrash", MoveItemToTrash);
  node::SetMethod(target, "beep", Beep);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_shell, atom::api::Shell::Initialize)
