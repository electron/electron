// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_base_window.h"

#include "electron/buildflags/buildflags.h"
#include "shell/browser/api/electron_api_view.h"
#include "shell/browser/native_window.h"
#include "shell/common/node_includes.h"

namespace {

// Converts binary data to Buffer.
v8::Local<v8::Value> ToBuffer(v8::Isolate* isolate, void* val, int size) {
  auto buffer = node::Buffer::Copy(isolate, static_cast<char*>(val), size);
  if (buffer.IsEmpty())
    return v8::Null(isolate);
  else
    return buffer.ToLocalChecked();
}

}  // namespace

namespace electron::api {

v8::Local<v8::Value> BaseWindow::GetNativeWindowHandle() {
  NSView* handle = window_->GetNativeWindowHandle().GetNativeNSView();
  return ToBuffer(isolate(), &handle, sizeof(handle));
}

}  // namespace electron::api
