// Copyright (c) 2026 Anthropic PBC
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extension_tab_util.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_function.h"
#include "extensions/common/extension.h"
#include "extensions/common/mojom/api_permission_id.mojom.h"
#include "extensions/common/permissions/permissions_data.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/extensions/electron_extension_tab_registry.h"
#include "url/gurl.h"

namespace extensions {

namespace {

constexpr char kNoSuchTabError[] = "No such tab";
constexpr char kNoCurrentWindowError[] = "No current window";
constexpr char kNoActiveTabError[] = "No active tab";

}  // namespace

bool IsTab(content::WebContents* web_contents) {
  if (!web_contents || !electron::api::WebContents::From(web_contents))
    return false;
  if (const auto* data =
          ElectronExtensionTabData::FromWebContents(web_contents))
    return data->is_tab();
  return ElectronExtensionTabRegistry::Get(web_contents->GetBrowserContext())
             ->mode() == ElectronTabMode::kAllWebContents;
}

electron::api::WebContents* GetElectronTabById(
    int tab_id,
    content::BrowserContext* browser_context) {
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents || !contents->web_contents())
    return nullptr;
  if (contents->web_contents()->GetBrowserContext() != browser_context)
    return nullptr;
  if (!IsTab(contents->web_contents()))
    return nullptr;
  return contents;
}

std::optional<int> GetCurrentWindowId(ExtensionFunction* function) {
  content::BrowserContext* context = function->browser_context();
  content::WebContents* sender = function->GetSenderWebContents();
  if (sender && sender->GetBrowserContext() == context) {
    if (const auto* data = ElectronExtensionTabData::FromWebContents(sender))
      return data->window_id();
  }
  return ElectronExtensionTabRegistry::Get(context)->focused_window_id();
}

base::expected<electron::api::WebContents*, std::string> ResolveTab(
    ExtensionFunction* function,
    std::optional<int> tab_id) {
  content::BrowserContext* context = function->browser_context();
  if (tab_id) {
    if (auto* contents = GetElectronTabById(*tab_id, context))
      return contents;
    return base::unexpected(kNoSuchTabError);
  }

  // A session that treats every WebContents as a tab has no tab strip to take
  // an active tab from.
  auto* registry = ElectronExtensionTabRegistry::Get(context);
  if (registry->mode() == ElectronTabMode::kAllWebContents)
    return base::unexpected(kNoSuchTabError);

  const std::optional<int> window_id = GetCurrentWindowId(function);
  if (!window_id)
    return base::unexpected(kNoCurrentWindowError);

  content::WebContents* active = registry->GetActiveTab(*window_id);
  auto* contents =
      IsTab(active) ? electron::api::WebContents::From(active) : nullptr;
  if (!contents)
    return base::unexpected(kNoActiveTabError);
  return contents;
}

bool CanAccessPrivilegedTabFields(const Extension* extension,
                                  electron::api::WebContents* contents) {
  const PermissionsData* permissions = extension->permissions_data();
  return permissions->HasAPIPermissionForTab(contents->ID(),
                                             mojom::APIPermissionID::kTab) ||
         permissions->HasHostPermission(
             contents->web_contents()->GetLastCommittedURL());
}

electron_api::tabs::Tab CreateTabObject(electron::api::WebContents* contents,
                                        const Extension* extension) {
  content::WebContents* web_contents = contents->web_contents();
  const auto* data = ElectronExtensionTabData::FromWebContents(web_contents);
  DCHECK(IsTab(web_contents));

  electron_api::tabs::Tab tab;
  if (data && data->is_tab()) {
    tab = data->CreateTab();
  } else {
    tab.active = contents->IsFocused();
    // Only a tab the app registered can be in a split view: the app reports
    // it as `splitViewId` through Extensions.addTab().
    tab.split_view_id = -1;
  }
  if (!data || !data->details().contains("lastAccessed")) {
    tab.last_accessed =
        web_contents->GetLastActiveTime().InMillisecondsFSinceUnixEpoch();
  }

  // Written after the registered fields so the app cannot spoof them.
  tab.id = contents->ID();
  tab.selected = tab.highlighted;
  tab.incognito = false;
  tab.audible = web_contents->IsCurrentlyAudible();
  tab.muted_info.emplace();
  tab.muted_info->muted = web_contents->IsAudioMuted();
  tab.muted_info->reason = electron_api::tabs::MutedInfoReason::kUser;
  tab.url = web_contents->GetLastCommittedURL().spec();
  tab.title = base::UTF16ToUTF8(web_contents->GetTitle());
  tab.pending_url.reset();
  tab.status = electron_api::tabs::TabStatus::kNone;
  tab.width.reset();
  tab.height.reset();

  if (extension && !CanAccessPrivilegedTabFields(extension, contents)) {
    tab.url.reset();
    tab.title.reset();
    tab.fav_icon_url.reset();
  }

  return tab;
}

}  // namespace extensions
