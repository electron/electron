// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_

#include <string>

#include "brightray/browser/browser_client.h"

namespace atom {

class AtomResourceDispatcherHostDelegate;

class AtomBrowserClient : public brightray::BrowserClient {
 public:
  AtomBrowserClient();
  virtual ~AtomBrowserClient();

 protected:
  // content::ContentBrowserClient:
  virtual void RenderProcessWillLaunch(
      content::RenderProcessHost* host) OVERRIDE;
  virtual void ResourceDispatcherHostCreated() OVERRIDE;
  virtual content::SpeechRecognitionManagerDelegate*
      GetSpeechRecognitionManagerDelegate() override;
  virtual content::AccessTokenStore* CreateAccessTokenStore() OVERRIDE;
  virtual void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                                   const GURL& url,
                                   WebPreferences* prefs) OVERRIDE;
  virtual bool ShouldSwapBrowsingInstancesForNavigation(
      content::SiteInstance* site_instance,
      const GURL& current_url,
      const GURL& new_url) OVERRIDE;
  virtual std::string GetApplicationLocale() OVERRIDE;
  virtual void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                              int child_process_id) OVERRIDE;

 private:
  virtual brightray::BrowserMainParts* OverrideCreateBrowserMainParts(
      const content::MainFunctionParams&) OVERRIDE;

  scoped_ptr<AtomResourceDispatcherHostDelegate> resource_dispatcher_delegate_;

  // The render process which would be swapped out soon.
  content::RenderProcessHost* dying_render_process_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
