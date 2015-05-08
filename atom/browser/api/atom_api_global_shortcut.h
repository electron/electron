// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_GLOBAL_SHORTCUT_H_
#define ATOM_BROWSER_API_ATOM_API_GLOBAL_SHORTCUT_H_

#include <map>
#include <string>

#include "base/callback.h"
#include "chrome/browser/extensions/global_shortcut_listener.h"
#include "native_mate/wrappable.h"
#include "native_mate/handle.h"
#include "ui/base/accelerators/accelerator.h"

namespace atom {

namespace api {

class GlobalShortcut : public extensions::GlobalShortcutListener::Observer,
                       public mate::Wrappable {
 public:
  static mate::Handle<GlobalShortcut> Create(v8::Isolate* isolate);

  typedef base::Callback<v8::Handle<v8::Value>(const bool)> KeyEventHandler;

 protected:
  GlobalShortcut();
  virtual ~GlobalShortcut();

  // mate::Wrappable implementations:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  typedef std::map<ui::Accelerator, KeyEventHandler> AcceleratorCallbackMap;

  bool Register(const ui::Accelerator& accelerator,
                const KeyEventHandler& callback);
  bool RegisterWithRelease(const ui::Accelerator& accelerator,
                           const KeyEventHandler& callback);
  bool IsRegistered(const ui::Accelerator& accelerator);
  void Unregister(const ui::Accelerator& accelerator);
  void UnregisterAll();

  // GlobalShortcutListener::Observer implementation.
  void OnKeyPressed(const ui::Accelerator& accelerator) override;
  void OnKeyReleased(const ui::Accelerator& accelerator) override;

  AcceleratorCallbackMap accelerator_callback_map_;
  AcceleratorCallbackMap accelerator_released_callback_map_;

  DISALLOW_COPY_AND_ASSIGN(GlobalShortcut);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_GLOBAL_SHORTCUT_H_
