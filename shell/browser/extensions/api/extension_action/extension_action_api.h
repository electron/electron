// Copyright (c) 2023 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_EXTENSIONS_API_EXTENSION_ACTION_EXTENSION_ACTION_API_H_
#define SHELL_BROWSER_EXTENSIONS_API_EXTENSION_ACTION_EXTENSION_ACTION_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {

// Implementation of the action API.
class ExtensionActionFunction : public ExtensionFunction {
 protected:
  ExtensionActionFunction();
  ~ExtensionActionFunction() override = default;

  // ExtensionFunction
  ResponseAction Run() override;

  virtual ResponseAction RunExtensionAction() = 0;
};

//
// Implementations of each extension action API.
//
// action bindings are created for these by extending them then declaring an
// EXTENSION_FUNCTION_NAME.
//

// show
class ExtensionActionShowFunction : public ExtensionActionFunction {
 protected:
  ~ExtensionActionShowFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

// hide
class ExtensionActionHideFunction : public ExtensionActionFunction {
 protected:
  ~ExtensionActionHideFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

// setIcon
class ExtensionActionSetIconFunction : public ExtensionActionFunction {
 protected:
  ~ExtensionActionSetIconFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

// setTitle
class ExtensionActionSetTitleFunction : public ExtensionActionFunction {
 protected:
  ~ExtensionActionSetTitleFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

// setPopup
class ExtensionActionSetPopupFunction : public ExtensionActionFunction {
 protected:
  ~ExtensionActionSetPopupFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

// setBadgeText
class ExtensionActionSetBadgeTextFunction : public ExtensionActionFunction {
 protected:
  ~ExtensionActionSetBadgeTextFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

// setBadgeBackgroundColor
class ExtensionActionSetBadgeBackgroundColorFunction
    : public ExtensionActionFunction {
 protected:
  ~ExtensionActionSetBadgeBackgroundColorFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

// getTitle
class ExtensionActionGetTitleFunction : public ExtensionActionFunction {
 protected:
  ~ExtensionActionGetTitleFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

// getPopup
class ExtensionActionGetPopupFunction : public ExtensionActionFunction {
 protected:
  ~ExtensionActionGetPopupFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

// getBadgeText
class ExtensionActionGetBadgeTextFunction : public ExtensionActionFunction {
 protected:
  ~ExtensionActionGetBadgeTextFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

// getBadgeBackgroundColor
class ExtensionActionGetBadgeBackgroundColorFunction
    : public ExtensionActionFunction {
 protected:
  ~ExtensionActionGetBadgeBackgroundColorFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

//
// action.* aliases for supported action APIs.
//

class ActionSetIconFunction : public ExtensionActionSetIconFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.setIcon", ACTION_SETICON)

 protected:
  ~ActionSetIconFunction() override = default;
};

class ActionGetPopupFunction : public ExtensionActionGetPopupFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.getPopup", ACTION_GETPOPUP)

 protected:
  ~ActionGetPopupFunction() override = default;
};

class ActionSetPopupFunction : public ExtensionActionSetPopupFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.setPopup", ACTION_SETPOPUP)

 protected:
  ~ActionSetPopupFunction() override = default;
};

class ActionGetTitleFunction : public ExtensionActionGetTitleFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.getTitle", ACTION_GETTITLE)

 protected:
  ~ActionGetTitleFunction() override = default;
};

class ActionSetTitleFunction : public ExtensionActionSetTitleFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.setTitle", ACTION_SETTITLE)

 protected:
  ~ActionSetTitleFunction() override = default;
};

class ActionGetBadgeTextFunction : public ExtensionActionGetBadgeTextFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.getBadgeText", ACTION_GETBADGETEXT)

 protected:
  ~ActionGetBadgeTextFunction() override = default;
};

class ActionSetBadgeTextFunction : public ExtensionActionSetBadgeTextFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.setBadgeText", ACTION_SETBADGETEXT)

 protected:
  ~ActionSetBadgeTextFunction() override = default;
};

class ActionGetBadgeBackgroundColorFunction
    : public ExtensionActionGetBadgeBackgroundColorFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.getBadgeBackgroundColor",
                             ACTION_GETBADGEBACKGROUNDCOLOR)

 protected:
  ~ActionGetBadgeBackgroundColorFunction() override = default;
};

class ActionSetBadgeBackgroundColorFunction
    : public ExtensionActionSetBadgeBackgroundColorFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.setBadgeBackgroundColor",
                             ACTION_SETBADGEBACKGROUNDCOLOR)

 protected:
  ~ActionSetBadgeBackgroundColorFunction() override = default;
};

class ActionGetBadgeTextColorFunction : public ExtensionActionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.getBadgeTextColor",
                             ACTION_GETBADGETEXTCOLOR)

 protected:
  ~ActionGetBadgeTextColorFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

class ActionSetBadgeTextColorFunction : public ExtensionActionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.setBadgeTextColor",
                             ACTION_SETBADGETEXTCOLOR)

 protected:
  ~ActionSetBadgeTextColorFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

class ActionEnableFunction : public ExtensionActionShowFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.enable", ACTION_ENABLE)

 protected:
  ~ActionEnableFunction() override = default;
};

class ActionDisableFunction : public ExtensionActionHideFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.disable", ACTION_DISABLE)

 protected:
  ~ActionDisableFunction() override = default;
};

class ActionIsEnabledFunction : public ExtensionActionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.isEnabled", ACTION_ISENABLED)

 protected:
  ~ActionIsEnabledFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

class ActionGetUserSettingsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.getUserSettings", ACTION_GETUSERSETTINGS)

  ActionGetUserSettingsFunction();
  ActionGetUserSettingsFunction(const ActionGetUserSettingsFunction&) = delete;
  ActionGetUserSettingsFunction& operator=(
      const ActionGetUserSettingsFunction&) = delete;

  ResponseAction Run() override;

 protected:
  ~ActionGetUserSettingsFunction() override;
};

class ActionOpenPopupFunction : public ExtensionActionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.openPopup", ACTION_OPENPOPUP)

 protected:
  ~ActionOpenPopupFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

}  // namespace extensions

#endif  // SHELL_BROWSER_EXTENSIONS_API_EXTENSION_ACTION_EXTENSION_ACTION_API_H_
