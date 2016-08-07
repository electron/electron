// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <map>
#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "chrome/browser/extensions/global_shortcut_listener.h"
#include "native_mate/handle.h"
#include "ui/base/accelerators/accelerator.h"

namespace atom {

namespace api {

class GlobalShortcut : public extensions::GlobalShortcutListener::Observer,
                       public mate::TrackableObject<GlobalShortcut> {
 public:
  static mate::Handle<GlobalShortcut> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit GlobalShortcut(v8::Isolate* isolate);
  ~GlobalShortcut() override;

 private:
  typedef std::map<ui::Accelerator, base::Closure> AcceleratorCallbackMap;

  bool Register(const ui::Accelerator& accelerator,
                const base::Closure& callback);
  bool IsRegistered(const ui::Accelerator& accelerator);
  void Unregister(const ui::Accelerator& accelerator);
  void UnregisterAll();

  // GlobalShortcutListener::Observer implementation.
  void OnKeyPressed(const ui::Accelerator& accelerator) override;

  AcceleratorCallbackMap accelerator_callback_map_;

  DISALLOW_COPY_AND_ASSIGN(GlobalShortcut);
};

}  // namespace api

}  // namespace atom
