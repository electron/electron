// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_app.h"

#include "browser/browser.h"
#include "vendor/node/src/node.h"

namespace atom {

namespace api {

// static
v8::Handle<v8::Value> App::Quit(const v8::Arguments &args) {
  v8::HandleScope scope;

  Browser::Get()->Quit();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::Terminate(const v8::Arguments &args) {
  v8::HandleScope scope;

  Browser::Get()->Terminate();

  return v8::Undefined();
}

// static
void App::Initialize(v8::Handle<v8::Object> target) {
  node::SetMethod(target, "quit", Quit);
  node::SetMethod(target, "terminate", Terminate);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_app, atom::api::App::Initialize)
