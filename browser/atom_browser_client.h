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
  virtual ~AtomBrowserClient();

 protected:
  net::URLRequestContextGetter* CreateRequestContext(
      content::BrowserContext* browser_context,
      content::ProtocolHandlerMap* protocol_handlers) OVERRIDE;
  virtual void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                                   const GURL& url,
                                   WebPreferences* prefs) OVERRIDE;
  virtual bool ShouldSwapProcessesForNavigation(
      content::SiteInstance* site_instance,
      const GURL& current_url,
      const GURL& new_url) OVERRIDE;

 private:
  virtual brightray::BrowserMainParts* OverrideCreateBrowserMainParts(
      const content::MainFunctionParams&) OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CLIENT_
