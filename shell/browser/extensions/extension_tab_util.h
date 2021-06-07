// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_EXTENSIONS_EXTENSION_TAB_UTIL_H_
#define SHELL_BROWSER_EXTENSIONS_EXTENSION_TAB_UTIL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "extensions/common/features/feature.h"
#include "shell/common/extensions/api/tabs.h"
#include "ui/base/window_open_disposition.h"

class GURL;
class ExtensionFunction;

namespace electron {
namespace api {
struct ExtensionTabDetails;
class WebContents;
}  // namespace api

}  // namespace electron

namespace base {
class DictionaryValue;
class ListValue;
}  // namespace base

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace gfx {
class Rect;
}

namespace extensions {
class Extension;

// Provides various utility functions that help manipulate tabs.
class ExtensionTabUtil {
 public:
  enum PopulateTabBehavior {
    kPopulateTabs,
    kDontPopulateTabs,
  };

  enum ScrubTabBehaviorType {
    kScrubTabFully,
    kScrubTabUrlToOrigin,
    kDontScrubTab,
  };

  struct ScrubTabBehavior {
    ScrubTabBehaviorType committed_info;
    ScrubTabBehaviorType pending_info;
  };

  static int GetTabId(content::WebContents* web_contents);

  // Gets the level of scrubbing of tab data that needs to happen for a given
  // extension and web contents. This is the preferred way to get
  // ScrubTabBehavior.
  static ScrubTabBehavior GetScrubTabBehavior(const Extension* extension,
                                              Feature::Context context,
                                              content::WebContents* contents);

  // Removes any privacy-sensitive fields from a Tab object if appropriate,
  // given the permissions of the extension and the tab in question.  The
  // tab object is modified in place.
  static void ScrubTabForExtension(const Extension* extension,
                                   content::WebContents* contents,
                                   api::tabs::Tab* tab,
                                   ScrubTabBehavior scrub_tab_behavior);

  static electron::api::WebContents* GetWebContentsById(int tab_id);

  static absl::optional<electron::api::ExtensionTabDetails>
  GetTabDetailsFromWebContents(electron::api::WebContents* contents);

  static std::unique_ptr<api::tabs::Tab> CreateTabObject(
      content::WebContents* contents,
      electron::api::ExtensionTabDetails tab,
      ScrubTabBehavior scrub_tab_behavior,
      const Extension* extension,
      int tab_index);

  static std::unique_ptr<api::tabs::Tab> CreateTabObject(
      content::WebContents* contents,
      electron::api::ExtensionTabDetails tab,
      int tab_index);
};

}  // namespace extensions

#endif  // SHELL_BROWSER_EXTENSIONS_EXTENSION_TAB_UTIL_H_
