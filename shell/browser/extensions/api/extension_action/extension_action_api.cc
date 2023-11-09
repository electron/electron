// Copyright (c) 2023 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/extension_action/extension_action_api.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/lazy_instance.h"
#include "base/observer_list.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/mojom/view_type.mojom.h"

using content::WebContents;

namespace extensions {

//
// ExtensionActionAPI::Observer
//

void ExtensionActionAPI::Observer::OnExtensionActionUpdated(
    ExtensionAction* extension_action,
    content::WebContents* web_contents,
    content::BrowserContext* browser_context) {}

void ExtensionActionAPI::Observer::OnExtensionActionAPIShuttingDown() {}

ExtensionActionAPI::Observer::~Observer() {}

//
// ExtensionActionAPI
//

static base::LazyInstance<BrowserContextKeyedAPIFactory<ExtensionActionAPI>>::
    DestructorAtExit g_extension_action_api_factory = LAZY_INSTANCE_INITIALIZER;

ExtensionActionAPI::ExtensionActionAPI(content::BrowserContext* context)
    : browser_context_(context), extension_prefs_(nullptr) {}

ExtensionActionAPI::~ExtensionActionAPI() {}

// static
BrowserContextKeyedAPIFactory<ExtensionActionAPI>*
ExtensionActionAPI::GetFactoryInstance() {
  return g_extension_action_api_factory.Pointer();
}

// static
ExtensionActionAPI* ExtensionActionAPI::Get(content::BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<ExtensionActionAPI>::Get(context);
}

ExtensionPrefs* ExtensionActionAPI::GetExtensionPrefs() {
  return nullptr;
}

void ExtensionActionAPI::Shutdown() {}

//
// ExtensionActionFunction
//

ExtensionActionFunction::ExtensionActionFunction() {}

ExtensionActionFunction::~ExtensionActionFunction() {}

ExtensionFunction::ResponseAction ExtensionActionFunction::Run() {
  return RunExtensionAction();
}

ExtensionFunction::ResponseAction
ExtensionActionShowFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.show is not supported in Electron";

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
ExtensionActionHideFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.hide is not supported in Electron";

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
ActionIsEnabledFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.isEnabled is not supported in Electron";

  return RespondNow(WithArguments(false));
}

ExtensionFunction::ResponseAction
ExtensionActionSetIconFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.setIcon is not supported in Electron";

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
ExtensionActionSetTitleFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.setTitle is not supported in Electron";

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
ExtensionActionSetPopupFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.setPopup is not supported in Electron";

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
ExtensionActionSetBadgeTextFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.setBadgeText is not supported in Electron";

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
ExtensionActionSetBadgeBackgroundColorFunction::RunExtensionAction() {
  LOG(INFO)
      << "chrome.action.setBadgeBackgroundColor is not supported in Electron";

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
ActionSetBadgeTextColorFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.setBadgeTextColor is not supported in Electron";

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
ExtensionActionGetTitleFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.getTitle is not supported in Electron";

  return RespondNow(WithArguments(""));
}

ExtensionFunction::ResponseAction
ExtensionActionGetPopupFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.getPopup is not supported in Electron";

  return RespondNow(WithArguments(""));
}

ExtensionFunction::ResponseAction
ExtensionActionGetBadgeTextFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.getBadgeText is not supported in Electron";

  return RespondNow(WithArguments(""));
}

ExtensionFunction::ResponseAction
ExtensionActionGetBadgeBackgroundColorFunction::RunExtensionAction() {
  LOG(INFO)
      << "chrome.action.getBadgeBackgroundColor is not supported in Electron";

  base::Value::List list;
  return RespondNow(WithArguments(std::move(list)));
}

ExtensionFunction::ResponseAction
ActionGetBadgeTextColorFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.getBadgeTextColor is not supported in Electron";

  base::Value::List list;
  return RespondNow(WithArguments(std::move(list)));
}

ActionGetUserSettingsFunction::ActionGetUserSettingsFunction() = default;
ActionGetUserSettingsFunction::~ActionGetUserSettingsFunction() = default;

ExtensionFunction::ResponseAction ActionGetUserSettingsFunction::Run() {
  LOG(INFO) << "chrome.action.getUserSettings is not supported in Electron";

  base::Value::Dict ui_settings;
  return RespondNow(WithArguments(std::move(ui_settings)));
}

ExtensionFunction::ResponseAction
ActionOpenPopupFunction::RunExtensionAction() {
  LOG(INFO) << "chrome.action.openPopup is not supported in Electron";

  return RespondNow(NoArguments());
}

}  // namespace extensions
