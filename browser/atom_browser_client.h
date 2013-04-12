// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CLIENT_
#define ATOM_BROWSER_ATOM_BROWSER_CLIENT_

#include "brightray/browser/browser_client.h"

namespace atom {

class AtomBrowserClient : public brightray::BrowserClient {
public:
  AtomBrowserClient();
  ~AtomBrowserClient();

protected:
  virtual void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                                   const GURL& url,
                                   webkit_glue::WebPreferences* prefs) OVERRIDE;

private:
  virtual brightray::BrowserMainParts* OverrideCreateBrowserMainParts(
      const content::MainFunctionParams&) OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CLIENT_
