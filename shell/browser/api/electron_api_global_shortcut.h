// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_GLOBAL_SHORTCUT_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_GLOBAL_SHORTCUT_H_

#include <map>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "extensions/common/extension_id.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/self_keep_alive.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/global_accelerator_listener/global_accelerator_listener.h"

namespace electron::api {

class GlobalShortcut final : private ui::GlobalAcceleratorListener::Observer,
                             public gin::Wrappable<GlobalShortcut> {
 public:
  static GlobalShortcut* Create(v8::Isolate* isolate);

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  // Make public for cppgc::MakeGarbageCollected.
  GlobalShortcut();
  ~GlobalShortcut() override;

  // disable copy
  GlobalShortcut(const GlobalShortcut&) = delete;
  GlobalShortcut& operator=(const GlobalShortcut&) = delete;

 private:
  typedef std::map<ui::Accelerator, base::RepeatingClosure>
      AcceleratorCallbackMap;
  typedef std::map<std::string, base::RepeatingClosure> CommandCallbackMap;

  bool RegisterAll(const std::vector<ui::Accelerator>& accelerators,
                   const base::RepeatingClosure& callback);
  bool Register(const ui::Accelerator& accelerator,
                const base::RepeatingClosure& callback);
  bool IsRegistered(const ui::Accelerator& accelerator);
  void Unregister(const ui::Accelerator& accelerator);
  void UnregisterSome(const std::vector<ui::Accelerator>& accelerators);
  void UnregisterAll();

  // GlobalAcceleratorListener::Observer implementation.
  void OnKeyPressed(const ui::Accelerator& accelerator) override;
  void ExecuteCommand(const extensions::ExtensionId& extension_id,
                      const std::string& command_id) override;

  AcceleratorCallbackMap accelerator_callback_map_;
  CommandCallbackMap command_callback_map_;

  gin_helper::SelfKeepAlive<GlobalShortcut> keep_alive_{this};
  base::WeakPtrFactory<GlobalShortcut> weak_ptr_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_GLOBAL_SHORTCUT_H_
