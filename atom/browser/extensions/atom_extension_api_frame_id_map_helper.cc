// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extension_api_frame_id_map_helper.h"

#include <stdint.h>
#include "atom/browser/extensions/tab_helper.h"
#include "base/bind.h"
#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"

namespace extensions {

AtomExtensionApiFrameIdMapHelper::AtomExtensionApiFrameIdMapHelper(
    ExtensionApiFrameIdMap* owner)
    : owner_(owner) {
  registrar_.Add(this, chrome::NOTIFICATION_TAB_PARENTED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

AtomExtensionApiFrameIdMapHelper::~AtomExtensionApiFrameIdMapHelper() {}

void AtomExtensionApiFrameIdMapHelper::GetTabAndWindowId(
    content::RenderFrameHost* rfh,
    int* tab_id,
    int* window_id) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(rfh);
  *tab_id = extensions::TabHelper::IdForTab(web_contents);
  *window_id = extensions::TabHelper::IdForWindowContainingTab(web_contents);
}

void AtomExtensionApiFrameIdMapHelper::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_TAB_PARENTED, type);
  content::WebContents* web_contents =
      content::Source<content::WebContents>(source).ptr();

  int32_t tab_id = extensions::TabHelper::IdForTab(web_contents);
  int32_t window_id =
    extensions::TabHelper::IdForWindowContainingTab(web_contents);

  if (tab_id == -1 || window_id == -1)
    return;

  web_contents->ForEachFrame(
      base::Bind(&ExtensionApiFrameIdMap::UpdateTabAndWindowId,
                 base::Unretained(owner_), tab_id, window_id));
}

}  // namespace extensions
