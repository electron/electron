// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_API_FRAME_ID_MAP_HELPER_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_API_FRAME_ID_MAP_HELPER_H_

#include "base/macros.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/extension_api_frame_id_map.h"

namespace extensions {

class AtomExtensionApiFrameIdMapHelper
    : public ExtensionApiFrameIdMapHelper,
      public content::NotificationObserver {
 public:
  explicit AtomExtensionApiFrameIdMapHelper(ExtensionApiFrameIdMap* owner);
  ~AtomExtensionApiFrameIdMapHelper() override;

 private:
  // AtomExtensionApiFrameIdMapHelper:
  void GetTabAndWindowId(content::RenderFrameHost* rfh,
                         int* tab_id_out,
                         int* window_id_out) override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  ExtensionApiFrameIdMap* owner_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionApiFrameIdMapHelper);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_API_FRAME_ID_MAP_HELPER_H_
