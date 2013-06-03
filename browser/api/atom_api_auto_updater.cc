// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_auto_updater.h"

#include "browser/auto_updater.h"

namespace atom {

namespace api {

AutoUpdater::AutoUpdater(v8::Handle<v8::Object> wrapper)
    : EventEmitter(wrapper) {
}

AutoUpdater::~AutoUpdater() {
}

// static
void AutoUpdater::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_auto_updater, atom::api::AutoUpdater::Initialize)
