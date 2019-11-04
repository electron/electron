// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_GLOBAL_SHORTCUT_H_
#define SHELL_BROWSER_API_ATOM_API_GLOBAL_SHORTCUT_H_

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "chrome/browser/extensions/global_shortcut_listener.h"
#include "gin/handle.h"
#include "shell/common/gin_helper/trackable_object.h"
#include "ui/base/accelerators/accelerator.h"

namespace electron {

namespace api {

class GlobalShortcut : public extensions::GlobalShortcutListener::Observer,
                       public gin_helper::TrackableObject<GlobalShortcut> {
 public:
  static gin::Handle<GlobalShortcut> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit GlobalShortcut(v8::Isolate* isolate);
  ~GlobalShortcut() override;

 private:
  typedef std::map<ui::Accelerator, base::Closure> AcceleratorCallbackMap;

  bool RegisterAll(const std::vector<ui::Accelerator>& accelerators,
                   const base::Closure& callback);
  bool Register(const ui::Accelerator& accelerator,
                const base::Closure& callback);
  bool IsRegistered(const ui::Accelerator& accelerator);
  void Unregister(const ui::Accelerator& accelerator);
  void UnregisterSome(const std::vector<ui::Accelerator>& accelerators);
  void UnregisterAll();

  // GlobalShortcutListener::Observer implementation.
  void OnKeyPressed(const ui::Accelerator& accelerator) override;

  AcceleratorCallbackMap accelerator_callback_map_;

  DISALLOW_COPY_AND_ASSIGN(GlobalShortcut);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_GLOBAL_SHORTCUT_H_
