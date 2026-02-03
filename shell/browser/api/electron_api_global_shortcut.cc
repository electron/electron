// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_global_shortcut.h"

#include <string>
#include <vector>

#include "base/containers/map_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/uuid.h"
#include "components/prefs/pref_service.h"
#include "electron/shell/browser/electron_browser_context.h"
#include "electron/shell/common/electron_constants.h"
#include "extensions/common/command.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/electron_api_system_preferences.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_converters/accelerator_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/node_includes.h"

#if BUILDFLAG(IS_MAC)
#include "base/mac/mac_util.h"
#endif

using extensions::Command;
using ui::GlobalAcceleratorListener;

namespace {

#if BUILDFLAG(IS_MAC)
bool RegisteringMediaKeyForUntrustedClient(const ui::Accelerator& accelerator) {
  return accelerator.IsMediaKey() &&
         !electron::api::SystemPreferences::IsTrustedAccessibilityClient(false);
}

bool MapHasMediaKeys(
    const std::map<ui::Accelerator, base::RepeatingClosure>& accelerator_map) {
  return std::ranges::any_of(
      accelerator_map, [](const auto& ac) { return ac.first.IsMediaKey(); });
}
#endif

}  // namespace

namespace electron::api {

gin::DeprecatedWrapperInfo GlobalShortcut::kWrapperInfo = {
    gin::kEmbedderNativeGin};

GlobalShortcut::GlobalShortcut() = default;

GlobalShortcut::~GlobalShortcut() {
  UnregisterAll();
}

void GlobalShortcut::OnKeyPressed(const ui::Accelerator& accelerator) {
  if (auto* cb = base::FindOrNull(accelerator_callback_map_, accelerator)) {
    cb->Run();
  } else {
    // This should never occur, because if it does,
    // ui::GlobalAcceleratorListener notifies us with wrong accelerator.
    NOTREACHED();
  }
}

void GlobalShortcut::ExecuteCommand(const extensions::ExtensionId& extension_id,
                                    const std::string& command_id) {
  if (auto* cb = base::FindOrNull(command_callback_map_, command_id)) {
    cb->Run();
  } else {
    // This should never occur, because if it does, GlobalAcceleratorListener
    // notifies us with wrong command.
    NOTREACHED();
  }
}

bool GlobalShortcut::RegisterAll(
    const std::vector<ui::Accelerator>& accelerators,
    const base::RepeatingClosure& callback) {
  if (!electron::Browser::Get()->is_ready()) {
    gin_helper::ErrorThrower(JavascriptEnvironment::GetIsolate())
        .ThrowError("globalShortcut cannot be used before the app is ready");
    return false;
  }
  std::vector<ui::Accelerator> registered;

  for (auto& accelerator : accelerators) {
    if (!Register(accelerator, callback)) {
      // Unregister all shortcuts if any failed.
      UnregisterSome(registered);
      return false;
    }

    registered.push_back(accelerator);
  }
  return true;
}

bool GlobalShortcut::Register(const ui::Accelerator& accelerator,
                              const base::RepeatingClosure& callback) {
  if (!electron::Browser::Get()->is_ready()) {
    gin_helper::ErrorThrower(JavascriptEnvironment::GetIsolate())
        .ThrowError("globalShortcut cannot be used before the app is ready");
    return false;
  }
#if BUILDFLAG(IS_MAC)
  if (accelerator.IsMediaKey()) {
    if (RegisteringMediaKeyForUntrustedClient(accelerator))
      return false;

    ui::GlobalAcceleratorListener::SetShouldUseInternalMediaKeyHandling(false);
  }
#endif

  auto* instance = ui::GlobalAcceleratorListener::GetInstance();
  if (!instance) {
    return false;
  }

  if (instance->IsRegistrationHandledExternally()) {
    auto* context = ElectronBrowserContext::GetDefaultBrowserContext();
    PrefService* prefs = context->prefs();

    // Need a unique profile id. Set one if not generated yet, otherwise re-use
    // the same so that the session for the globalShortcuts is able to get
    // already registered shortcuts from the previous session. This will be used
    // by GlobalAcceleratorListenerLinux as a session key.
    std::string profile_id = prefs->GetString(kElectronGlobalShortcutsUuid);
    if (profile_id.empty()) {
      profile_id = base::Uuid::GenerateRandomV4().AsLowercaseString();
      prefs->SetString(kElectronGlobalShortcutsUuid, profile_id);
    }

    // There is no way to get command id for the accelerator as it's extensions'
    // thing. Instead, we can convert it to string in a following example form
    // - std::string("Alt+Shift+K"). That must be sufficient enough for us to
    // map this accelerator with registered commands.
    const std::string command_str =
        extensions::Command::AcceleratorToString(accelerator);
    ui::CommandMap commands;
    extensions::Command command(
        command_str, base::UTF8ToUTF16("Electron shortcut " + command_str),
        /*accelerator=*/std::string(), /*global=*/true);
    command.set_accelerator(accelerator);
    commands[command_str] = command;

    // In order to distinguish the shortcuts, we must register multiple commands
    // as different extensions. Otherwise, each shortcut will be an alternative
    // for the very first registered and we'll not be able to distinguish them.
    // For example, if Alt+Shift+K is registered first, registering and pressing
    // Alt+Shift+M will trigger global shortcuts, but the command id that is
    // received by GlobalShortcut will correspond to Alt+Shift+K as our command
    // id is basically a stringified accelerator.
    const std::string fake_extension_id = command_str + "+" + profile_id;
    instance->OnCommandsChanged(fake_extension_id, profile_id, commands,
                                gfx::kNullAcceleratedWidget, this);
    command_callback_map_[command_str] = callback;
    return true;
  } else {
    if (instance->RegisterAccelerator(accelerator, this)) {
      accelerator_callback_map_[accelerator] = callback;
      return true;
    }
  }
  return false;
}

void GlobalShortcut::Unregister(const ui::Accelerator& accelerator) {
  if (!electron::Browser::Get()->is_ready()) {
    gin_helper::ErrorThrower(JavascriptEnvironment::GetIsolate())
        .ThrowError("globalShortcut cannot be used before the app is ready");
    return;
  }
  if (accelerator_callback_map_.erase(accelerator) == 0)
    return;

#if BUILDFLAG(IS_MAC)
  if (accelerator.IsMediaKey() && !MapHasMediaKeys(accelerator_callback_map_)) {
    ui::GlobalAcceleratorListener::SetShouldUseInternalMediaKeyHandling(true);
  }
#endif

  if (ui::GlobalAcceleratorListener::GetInstance()) {
    ui::GlobalAcceleratorListener::GetInstance()->UnregisterAccelerator(
        accelerator, this);
  }
}

void GlobalShortcut::UnregisterSome(
    const std::vector<ui::Accelerator>& accelerators) {
  for (auto& accelerator : accelerators) {
    Unregister(accelerator);
  }
}

bool GlobalShortcut::IsRegistered(const ui::Accelerator& accelerator) {
  if (accelerator_callback_map_.contains(accelerator)) {
    return true;
  }
  const std::string command_str =
      extensions::Command::AcceleratorToString(accelerator);
  return command_callback_map_.contains(command_str);
}

void GlobalShortcut::UnregisterAll() {
  if (!electron::Browser::Get()->is_ready()) {
    gin_helper::ErrorThrower(JavascriptEnvironment::GetIsolate())
        .ThrowError("globalShortcut cannot be used before the app is ready");
    return;
  }
  accelerator_callback_map_.clear();
  if (ui::GlobalAcceleratorListener::GetInstance()) {
    ui::GlobalAcceleratorListener::GetInstance()->UnregisterAccelerators(this);
  }
}

// static
gin_helper::Handle<GlobalShortcut> GlobalShortcut::Create(
    v8::Isolate* isolate) {
  return gin_helper::CreateHandle(isolate, new GlobalShortcut());
}

// static
gin::ObjectTemplateBuilder GlobalShortcut::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::DeprecatedWrappable<
             GlobalShortcut>::GetObjectTemplateBuilder(isolate)
      .SetMethod("registerAll", &GlobalShortcut::RegisterAll)
      .SetMethod("register", &GlobalShortcut::Register)
      .SetMethod("isRegistered", &GlobalShortcut::IsRegistered)
      .SetMethod("unregister", &GlobalShortcut::Unregister)
      .SetMethod("unregisterAll", &GlobalShortcut::UnregisterAll);
}

const char* GlobalShortcut::GetTypeName() {
  return "GlobalShortcut";
}

}  // namespace electron::api

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin::Dictionary dict{isolate, exports};
  dict.Set("globalShortcut", electron::api::GlobalShortcut::Create(isolate));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_global_shortcut, Initialize)
