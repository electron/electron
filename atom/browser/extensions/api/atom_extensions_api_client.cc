// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/api/atom_extensions_api_client.h"

#include <string>
#include "atom/browser/extensions/atom_extension_web_contents_observer.h"
#include "atom/browser/extensions/tab_helper.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/api/management/management_api_delegate.h"
#include "extensions/browser/api/web_request/web_request_event_details.h"
#include "extensions/browser/api/web_request/web_request_event_router_delegate.h"
#include "extensions/browser/requirements_checker.h"
#include "extensions/common/manifest_handlers/icons_handler.h"


namespace extensions {

class AtomRequirementsChecker : public RequirementsChecker {
 public:
  AtomRequirementsChecker() {}
  ~AtomRequirementsChecker() override {}

  void Check(const scoped_refptr<const Extension>& extension,
                     const RequirementsCheckedCallback& callback) override {
    NOTIMPLEMENTED();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomRequirementsChecker);
};


class AtomManagementAPIDelegate : public ManagementAPIDelegate {
 public:
  AtomManagementAPIDelegate() {}
  ~AtomManagementAPIDelegate() override {}

  bool LaunchAppFunctionDelegate(
      const Extension* extension,
      content::BrowserContext* context) const override { return false; }
  bool IsNewBookmarkAppsEnabled() const override { return false; }
  bool CanHostedAppsOpenInWindows() const override { return false; }
  GURL GetFullLaunchURL(const Extension* extension) const override {
    NOTIMPLEMENTED();
    return GURL();
  }
  LaunchType GetLaunchType(const ExtensionPrefs* prefs,
                                   const Extension* extension) const override {
    NOTIMPLEMENTED();
    return LaunchType::LAUNCH_TYPE_INVALID;
  }
  void GetPermissionWarningsByManifestFunctionDelegate(
      ManagementGetPermissionWarningsByManifestFunction* function,
      const std::string& manifest_str) const override {
    NOTIMPLEMENTED();
  }
  scoped_ptr<InstallPromptDelegate> SetEnabledFunctionDelegate(
      content::WebContents* web_contents,
      content::BrowserContext* browser_context,
      const Extension* extension,
      const base::Callback<void(bool)>& callback) const override {
    NOTIMPLEMENTED();
    return base::WrapUnique(install_prompt_delegate_);
  }
  scoped_ptr<RequirementsChecker> CreateRequirementsChecker() const override {
    return base::WrapUnique(new AtomRequirementsChecker());
  }

  // Enables the extension identified by |extension_id|.
  void EnableExtension(content::BrowserContext* context,
                               const std::string& extension_id) const override {
    NOTIMPLEMENTED();
  }

  // Disables the extension identified by |extension_id|.
  void DisableExtension(
      content::BrowserContext* context,
      const std::string& extension_id,
      Extension::DisableReason disable_reason) const override {
    NOTIMPLEMENTED();
  }

  // Used to show a confirmation dialog when uninstalling |target_extension|.
  scoped_ptr<UninstallDialogDelegate> UninstallFunctionDelegate(
      ManagementUninstallFunctionBase* function,
      const Extension* target_extension,
      bool show_programmatic_uninstall_ui) const override {
    NOTIMPLEMENTED();
    return base::WrapUnique(uninstall_dialog_delegate_);
  }

  // Uninstalls the extension.
  bool UninstallExtension(content::BrowserContext* context,
                                  const std::string& transient_extension_id,
                                  UninstallReason reason,
                                  const base::Closure& deletion_done_callback,
                                  base::string16* error) const override {
    NOTIMPLEMENTED();
    return false;
  }

  // Creates an app shortcut.
  bool CreateAppShortcutFunctionDelegate(
      ManagementCreateAppShortcutFunction* function,
      const Extension* extension) const override {
    NOTIMPLEMENTED();
    return false;
  }

  // Forwards the call to launch_util::SetLaunchType in chrome.
  void SetLaunchType(content::BrowserContext* context,
                             const std::string& extension_id,
                             LaunchType launch_type) const override {
    NOTIMPLEMENTED();
  }

  // Creates a bookmark app for |launch_url|.
  scoped_ptr<AppForLinkDelegate> GenerateAppForLinkFunctionDelegate(
      ManagementGenerateAppForLinkFunction* function,
      content::BrowserContext* context,
      const std::string& title,
      const GURL& launch_url) const override {
    NOTIMPLEMENTED();
    return base::WrapUnique(app_for_link_delegate_);
  }

  GURL GetIconURL(
      const extensions::Extension* extension,
      int icon_size,
      ExtensionIconSet::MatchType match,
      bool grayscale,
      bool* exists) const override {
    if (exists) {
      *exists = IconsInfo::GetIconURL(extension, icon_size, match)
          != GURL::EmptyGURL();
    }

    GURL icon_url(base::StringPrintf("%s%s/%d/%d%s",
                                     "chrome://extension-icon/",
                                     extension->id().c_str(),
                                     icon_size,
                                     match,
                                     grayscale ? "?grayscale=true" : ""));

    CHECK(icon_url.is_valid());
    return icon_url;
  }


 private:
  InstallPromptDelegate* install_prompt_delegate_;
  UninstallDialogDelegate* uninstall_dialog_delegate_;
  AppForLinkDelegate* app_for_link_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AtomManagementAPIDelegate);
};

AtomExtensionsAPIClient::AtomExtensionsAPIClient() {
}

void AtomExtensionsAPIClient::AttachWebContentsHelpers(
    content::WebContents* web_contents) const {
  extensions::TabHelper::CreateForWebContents(web_contents);
  AtomExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

WebViewGuestDelegate* AtomExtensionsAPIClient::CreateWebViewGuestDelegate(
    WebViewGuest* web_view_guest) const {
  return ExtensionsAPIClient::CreateWebViewGuestDelegate(web_view_guest);
}

WebRequestEventRouterDelegate*
AtomExtensionsAPIClient::CreateWebRequestEventRouterDelegate() const {
  return new extensions::WebRequestEventRouterDelegate();
}

ManagementAPIDelegate* AtomExtensionsAPIClient::CreateManagementAPIDelegate()
    const {
  return new AtomManagementAPIDelegate();
}

}  // namespace extensions
