// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/locker.h"

namespace gin_helper {

Locker::Locker(v8::Isolate* isolate) {
  if (IsBrowserProcess())
    locker_ = std::make_unique<v8::Locker>(isolate);
}

Locker::~Locker() = default;

void Locker::SetIsBrowserProcess(bool is_browser_process) {
  g_is_browser_process = is_browser_process;
}

void Locker::SetIsRendererProcess(bool is_renderer_process) {
  g_is_renderer_process = is_renderer_process;
}

bool Locker::g_is_browser_process = false;
bool Locker::g_is_renderer_process = false;

}  // namespace gin_helper
