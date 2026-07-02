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
#include "gin/per_isolate_data.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/global_accelerator_listener/global_accelerator_listener.h"

namespace electron::api {

class GlobalShortcut final
    : public gin::Wrappable<GlobalShortcut>,
      public gin_helper::EventEmitterMixin<GlobalShortcut>,
      private ui::GlobalAcceleratorListener::Observer,
      public gin::PerIsolateData::DisposeObserver {
 public:
  static GlobalShortcut* Create(v8::Isolate* isolate);

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  void Trace(cppgc::Visitor* visitor) const override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetClassName() const { return "GlobalShortcut"; }

  // gin::PerIsolateData::DisposeObserver
  void OnBeforeDispose(v8::Isolate* isolate) override {}
  void OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) override;
  void OnDisposed() override {}

  // Make public for cppgc::MakeGarbageCollected.
  explicit GlobalShortcut(v8::Isolate* isolate);
  ~GlobalShortcut() override;

  // disable copy
  GlobalShortcut(const GlobalShortcut&) = delete;
  GlobalShortcut& operator=(const GlobalShortcut&) = delete;

 private:
  typedef std::map<ui::Accelerator, base::RepeatingClosure>
      AcceleratorCallbackMap;
  typedef std::map<std::string, base::RepeatingClosure> CommandCallbackMap;

  void Dispose();
  // These take the accelerator as the string the app passed so that
  // registration-resolved can report it back verbatim.
  bool RegisterAll(const std::vector<std::string>& accelerators,
                   const base::RepeatingClosure& callback);
  bool Register(const std::string& accelerator_str,
                const base::RepeatingClosure& callback);
  bool IsRegistered(const ui::Accelerator& accelerator);
  void Unregister(const ui::Accelerator& accelerator);
  void UnregisterSome(const std::vector<ui::Accelerator>& accelerators);
  void UnregisterAll();
  void UnregisterAllInternal();
  void SetSuspended(bool suspend);
  bool IsSuspended();

  // GlobalAcceleratorListener::Observer implementation.
  void OnKeyPressed(const ui::Accelerator& accelerator) override;
  void ExecuteCommand(const extensions::ExtensionId& extension_id,
                      const std::string& command_id) override;

  // Called when an externally-handled registration (Wayland portal) resolves.
  void OnRegistrationResolved(const std::string& accelerator_group_id,
                              bool bound);
  void EmitRegistrationResolved(const std::string& accelerator, bool bound);

  AcceleratorCallbackMap accelerator_callback_map_;
  CommandCallbackMap command_callback_map_;
  // Maps accelerator group ids to the accelerator string the app registered.
  // Also scopes registration-resolved to shortcuts registered via this module.
  std::map<std::string, std::string> registered_accelerators_;
  bool is_disposed_ = false;
  bool registration_resolved_callback_set_ = false;

  gin::WeakCellFactory<GlobalShortcut> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_GLOBAL_SHORTCUT_H_
