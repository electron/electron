// Copyright (c) 2023 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_EXTENSIONS_API_EXTENSION_ACTION_EXTENSION_ACTION_API_H_
#define SHELL_BROWSER_EXTENSIONS_API_EXTENSION_ACTION_EXTENSION_ACTION_API_H_

#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_action.h"
#include "extensions/browser/extension_function.h"
#include "extensions/browser/extension_host_registry.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace extensions {

class ExtensionHost;
class ExtensionPrefs;

class ExtensionActionAPI : public BrowserContextKeyedAPI {
 public:
  class Observer {
   public:
    virtual void OnExtensionActionUpdated(
        ExtensionAction* extension_action,
        content::WebContents* web_contents,
        content::BrowserContext* browser_context);

    virtual void OnExtensionActionAPIShuttingDown();

   protected:
    virtual ~Observer() = default;
  };

  explicit ExtensionActionAPI(content::BrowserContext* context);

  ExtensionActionAPI(const ExtensionActionAPI&) = delete;
  ExtensionActionAPI& operator=(const ExtensionActionAPI&) = delete;

  ~ExtensionActionAPI() override = default;

  // Convenience method to get the instance for a profile.
  static ExtensionActionAPI* Get(content::BrowserContext* context);

  static BrowserContextKeyedAPIFactory<ExtensionActionAPI>*
  GetFactoryInstance();

  // Add or remove observers.
  void AddObserver(Observer* observer) {}
  void RemoveObserver(Observer* observer) {}

  // Notifies that there has been a change in the given |extension_action|.
  void NotifyChange(ExtensionAction* extension_action,
                    content::WebContents* web_contents,
                    content::BrowserContext* browser_context) {}

  // Dispatches the onClicked event for extension that owns the given action.
  void DispatchExtensionActionClicked(const ExtensionAction& extension_action,
                                      content::WebContents* web_contents,
                                      const Extension* extension) {}

  // Clears the values for all ExtensionActions for the tab associated with the
  // given |web_contents| (and signals that page actions changed).
  void ClearAllValuesForTab(content::WebContents* web_contents) {}

 private:
  friend class BrowserContextKeyedAPIFactory<ExtensionActionAPI>;

  // BrowserContextKeyedAPI implementation.
  void Shutdown() override;
  static const char* service_name() { return "ExtensionActionAPI"; }
  static const bool kServiceRedirectedInIncognito = true;
};

// Implementation of the browserAction and pageAction APIs.
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
// pageAction and browserAction bindings are created for these by extending them
// then declaring an EXTENSION_FUNCTION_NAME.
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
 public:
  static void SetReportErrorForInvisibleIconForTesting(bool value);

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

// openPopup
class ExtensionActionOpenPopupFunction : public ExtensionActionFunction {
 protected:
  ~ExtensionActionOpenPopupFunction() override = default;
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

class ActionOpenPopupFunction : public ExtensionActionOpenPopupFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("action.openPopup", ACTION_OPENPOPUP)

 protected:
  ~ActionOpenPopupFunction() override = default;
  ResponseAction RunExtensionAction() override;
};

//
// browserAction.* aliases for supported browserAction APIs.
//

class BrowserActionSetIconFunction : public ExtensionActionSetIconFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.setIcon", BROWSERACTION_SETICON)

 protected:
  ~BrowserActionSetIconFunction() override = default;
};

class BrowserActionSetTitleFunction : public ExtensionActionSetTitleFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.setTitle", BROWSERACTION_SETTITLE)

 protected:
  ~BrowserActionSetTitleFunction() override = default;
};

class BrowserActionSetPopupFunction : public ExtensionActionSetPopupFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.setPopup", BROWSERACTION_SETPOPUP)

 protected:
  ~BrowserActionSetPopupFunction() override = default;
};

class BrowserActionGetTitleFunction : public ExtensionActionGetTitleFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.getTitle", BROWSERACTION_GETTITLE)

 protected:
  ~BrowserActionGetTitleFunction() override = default;
};

class BrowserActionGetPopupFunction : public ExtensionActionGetPopupFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.getPopup", BROWSERACTION_GETPOPUP)

 protected:
  ~BrowserActionGetPopupFunction() override = default;
};

class BrowserActionSetBadgeTextFunction
    : public ExtensionActionSetBadgeTextFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.setBadgeText",
                             BROWSERACTION_SETBADGETEXT)

 protected:
  ~BrowserActionSetBadgeTextFunction() override = default;
};

class BrowserActionSetBadgeBackgroundColorFunction
    : public ExtensionActionSetBadgeBackgroundColorFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.setBadgeBackgroundColor",
                             BROWSERACTION_SETBADGEBACKGROUNDCOLOR)

 protected:
  ~BrowserActionSetBadgeBackgroundColorFunction() override = default;
};

class BrowserActionGetBadgeTextFunction
    : public ExtensionActionGetBadgeTextFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.getBadgeText",
                             BROWSERACTION_GETBADGETEXT)

 protected:
  ~BrowserActionGetBadgeTextFunction() override = default;
};

class BrowserActionGetBadgeBackgroundColorFunction
    : public ExtensionActionGetBadgeBackgroundColorFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.getBadgeBackgroundColor",
                             BROWSERACTION_GETBADGEBACKGROUNDCOLOR)

 protected:
  ~BrowserActionGetBadgeBackgroundColorFunction() override = default;
};

class BrowserActionEnableFunction : public ExtensionActionShowFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.enable", BROWSERACTION_ENABLE)

 protected:
  ~BrowserActionEnableFunction() override = default;
};

class BrowserActionDisableFunction : public ExtensionActionHideFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.disable", BROWSERACTION_DISABLE)

 protected:
  ~BrowserActionDisableFunction() override = default;
};

class BrowserActionOpenPopupFunction : public ExtensionActionOpenPopupFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("browserAction.openPopup",
                             BROWSERACTION_OPEN_POPUP)

 protected:
  ~BrowserActionOpenPopupFunction() override = default;
};

}  // namespace extensions

//
// pageAction.* aliases for supported pageAction APIs.
//

class PageActionShowFunction : public extensions::ExtensionActionShowFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pageAction.show", PAGEACTION_SHOW)

 protected:
  ~PageActionShowFunction() override = default;
};

class PageActionHideFunction : public extensions::ExtensionActionHideFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pageAction.hide", PAGEACTION_HIDE)

 protected:
  ~PageActionHideFunction() override = default;
};

class PageActionSetIconFunction
    : public extensions::ExtensionActionSetIconFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pageAction.setIcon", PAGEACTION_SETICON)

 protected:
  ~PageActionSetIconFunction() override = default;
};

class PageActionSetTitleFunction
    : public extensions::ExtensionActionSetTitleFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pageAction.setTitle", PAGEACTION_SETTITLE)

 protected:
  ~PageActionSetTitleFunction() override = default;
};

class PageActionSetPopupFunction
    : public extensions::ExtensionActionSetPopupFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pageAction.setPopup", PAGEACTION_SETPOPUP)

 protected:
  ~PageActionSetPopupFunction() override = default;
};

class PageActionGetTitleFunction
    : public extensions::ExtensionActionGetTitleFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pageAction.getTitle", PAGEACTION_GETTITLE)

 protected:
  ~PageActionGetTitleFunction() override = default;
};

class PageActionGetPopupFunction
    : public extensions::ExtensionActionGetPopupFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pageAction.getPopup", PAGEACTION_GETPOPUP)

 protected:
  ~PageActionGetPopupFunction() override = default;
};

#endif  // SHELL_BROWSER_EXTENSIONS_API_EXTENSION_ACTION_EXTENSION_ACTION_API_H_
