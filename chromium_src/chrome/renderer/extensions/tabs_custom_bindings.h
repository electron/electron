// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_EXTENSIONS_TABS_CUSTOM_BINDINGS_H_
#define CHROME_RENDERER_EXTENSIONS_TABS_CUSTOM_BINDINGS_H_

#include "extensions/renderer/object_backed_native_handler.h"

namespace extensions {

// Implements custom bindings for the tabs API.
class TabsCustomBindings : public ObjectBackedNativeHandler {
 public:
  explicit TabsCustomBindings(ScriptContext* context);

 private:
  // Creates a new messaging channel to the tab with the given ID.
  void OpenChannelToTab(const v8::FunctionCallbackInfo<v8::Value>& args);
};

}  // namespace extensions

#endif  // CHROME_RENDERER_EXTENSIONS_TABS_CUSTOM_BINDINGS_H_
