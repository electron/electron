// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/extension_tab_util.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "base/stl_util.h"
#include "base/strings/pattern.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/common/webui_url_constants.h"
#include "components/url_formatter/url_fixer.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "electron/shell/common/extensions/api/tabs.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/permissions_data.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/native_window.h"
#include "shell/browser/web_contents_zoom_controller.h"
#include "shell/browser/window_list.h"
#include "third_party/blink/public/common/page/page_zoom.h"

using content::NavigationEntry;
using content::WebContents;
using extensions::mojom::APIPermissionID;

namespace extensions {

ExtensionTabUtil::ScrubTabBehaviorType GetScrubTabBehaviorImpl(
    const Extension* extension,
    Feature::Context context,
    const GURL& url,
    int tab_id) {
  if (context == Feature::Context::WEBUI_CONTEXT) {
    return ExtensionTabUtil::kDontScrubTab;
  }

  if (context == Feature::Context::WEBUI_UNTRUSTED_CONTEXT) {
    return ExtensionTabUtil::kScrubTabFully;
  }

  bool has_permission = false;

  if (extension) {
    bool api_permission = false;
    if (tab_id == -1) {
      api_permission = extension->permissions_data()->HasAPIPermission(
          APIPermissionID::kTab);
    } else {
      api_permission = extension->permissions_data()->HasAPIPermissionForTab(
          tab_id, APIPermissionID::kTab);
    }

    bool host_permission = extension->permissions_data()
                               ->active_permissions()
                               .HasExplicitAccessToOrigin(url);
    has_permission = api_permission || host_permission;
  }

  if (!has_permission) {
    return ExtensionTabUtil::kScrubTabFully;
  }

  return ExtensionTabUtil::kDontScrubTab;
}

ExtensionTabUtil::ScrubTabBehavior ExtensionTabUtil::GetScrubTabBehavior(
    const Extension* extension,
    Feature::Context context,
    content::WebContents* contents) {
  auto* api_wc = electron::api::WebContents::From(contents);

  ScrubTabBehavior behavior;
  behavior.committed_info = GetScrubTabBehaviorImpl(
      extension, context, contents->GetLastCommittedURL(), api_wc->ID());
  NavigationEntry* entry = contents->GetController().GetPendingEntry();
  GURL pending_url;
  if (entry) {
    pending_url = entry->GetVirtualURL();
  }
  behavior.pending_info =
      GetScrubTabBehaviorImpl(extension, context, pending_url, api_wc->ID());
  return behavior;
}

void ExtensionTabUtil::ScrubTabForExtension(
    const Extension* extension,
    content::WebContents* contents,
    api::tabs::Tab* tab,
    ExtensionTabUtil::ScrubTabBehavior scrub_tab_behavior) {
  // Remove sensitive committed tab info if necessary.
  switch (scrub_tab_behavior.committed_info) {
    case kScrubTabFully:
      tab->url.reset();
      tab->title.reset();
      tab->fav_icon_url.reset();
      break;
    case kScrubTabUrlToOrigin:
      tab->url =
          std::make_unique<std::string>(GURL(*tab->url).GetOrigin().spec());
      break;
    case kDontScrubTab:
      break;
  }

  // Remove sensitive pending tab info if necessary.
  if (tab->pending_url) {
    switch (scrub_tab_behavior.pending_info) {
      case kScrubTabFully:
        tab->pending_url.reset();
        break;
      case kScrubTabUrlToOrigin:
        tab->pending_url = std::make_unique<std::string>(
            GURL(*tab->pending_url).GetOrigin().spec());
        break;
      case kDontScrubTab:
        break;
    }
  }
}

electron::api::WebContents* ExtensionTabUtil::GetWebContentsById(int tab_id) {
  return electron::api::WebContents::FromID(tab_id);
}

absl::optional<electron::api::ExtensionTabDetails>
ExtensionTabUtil::GetTabDetailsFromWebContents(
    electron::api::WebContents* contents) {
  if (!contents)
    return absl::nullopt;
  return electron::api::Session::FromBrowserContext(
             contents->web_contents()->GetBrowserContext())
      ->GetExtensionTabDetails(contents);
}

int ExtensionTabUtil::GetTabId(WebContents* web_contents) {
  auto* contents = electron::api::WebContents::From(web_contents);
  return contents ? contents->ID() : -1;
}

std::unique_ptr<api::tabs::MutedInfo> CreateMutedInfo(
    content::WebContents* contents,
    std::string reason,
    std::string extension_id) {
  DCHECK(contents);
  auto info = std::make_unique<api::tabs::MutedInfo>();
  info->muted = contents->IsAudioMuted();
  if (reason == "user") {
    info->reason = api::tabs::MUTED_INFO_REASON_USER;
  } else if (reason == "capture") {
    info->reason = api::tabs::MUTED_INFO_REASON_CAPTURE;
  } else if (reason == "extension") {
    info->reason = api::tabs::MUTED_INFO_REASON_EXTENSION;
    info->extension_id = std::make_unique<std::string>(extension_id);
  }
  return info;
}

bool HasValidMainFrameProcess(content::WebContents* contents) {
  content::RenderFrameHost* main_frame_host = contents->GetMainFrame();
  content::RenderProcessHost* process_host = main_frame_host->GetProcess();
  return process_host->IsReady() && process_host->IsInitializedAndNotDead();
}

api::tabs::TabStatus GetLoadingStatus(WebContents* contents) {
  if (contents->IsLoading())
    return api::tabs::TAB_STATUS_LOADING;

  // Anything that isn't backed by a process is considered unloaded.
  if (!HasValidMainFrameProcess(contents))
    return api::tabs::TAB_STATUS_UNLOADED;

  // Otherwise its considered loaded.
  return api::tabs::TAB_STATUS_COMPLETE;
}

std::unique_ptr<api::tabs::Tab> ExtensionTabUtil::CreateTabObject(
    content::WebContents* contents,
    electron::api::ExtensionTabDetails tab,
    int tab_index) {
  auto tab_object = std::make_unique<api::tabs::Tab>();
  tab_object->id = std::make_unique<int>(GetTabId(contents));
  tab_object->index = tab_index;
  tab_object->window_id = tab.window_id;
  tab_object->active = tab.active;
  tab_object->selected = tab.active;
  tab_object->highlighted = tab.highlighted;
  tab_object->pinned = tab.pinned;
  tab_object->status = GetLoadingStatus(contents);
  tab_object->group_id = tab.group_id;

  tab_object->audible = std::make_unique<bool>(contents->IsCurrentlyAudible());
  tab_object->discarded = tab.discarded;
  tab_object->auto_discardable = tab.auto_discardable;

  tab_object->muted_info =
      CreateMutedInfo(contents, tab.muted_reason, tab.muted_extension_id);

  tab_object->incognito = contents->GetBrowserContext()->IsOffTheRecord();
  gfx::Size contents_size = contents->GetContainerBounds().size();
  tab_object->width = std::make_unique<int>(contents_size.width());
  tab_object->height = std::make_unique<int>(contents_size.height());

  tab_object->url =
      std::make_unique<std::string>(contents->GetLastCommittedURL().spec());
  NavigationEntry* pending_entry = contents->GetController().GetPendingEntry();
  if (pending_entry) {
    tab_object->pending_url =
        std::make_unique<std::string>(pending_entry->GetVirtualURL().spec());
  }
  tab_object->title =
      std::make_unique<std::string>(base::UTF16ToUTF8(contents->GetTitle()));
  // TODO(tjudkins) This should probably use the LastCommittedEntry() for
  // consistency.
  NavigationEntry* visible_entry = contents->GetController().GetVisibleEntry();
  if (visible_entry && visible_entry->GetFavicon().valid) {
    tab_object->fav_icon_url =
        std::make_unique<std::string>(visible_entry->GetFavicon().url.spec());
  }
  tab_object->opener_tab_id = std::make_unique<int>(tab.opener_tab_id);

  return tab_object;
}

std::unique_ptr<api::tabs::Tab> ExtensionTabUtil::CreateTabObject(
    content::WebContents* contents,
    electron::api::ExtensionTabDetails tab,
    ScrubTabBehavior scrub_tab_behavior,
    const Extension* extension,
    int tab_index) {
  auto tab_object = CreateTabObject(contents, tab, tab_index);
  ScrubTabForExtension(extension, contents, tab_object.get(),
                       scrub_tab_behavior);
  return tab_object;
}

}  // namespace extensions
