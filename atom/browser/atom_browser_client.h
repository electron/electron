// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_

#include <string>

#include "brightray/browser/browser_client.h"

namespace content {
class QuotaPermissionContext;
}

namespace atom {

class AtomResourceDispatcherHostDelegate;

class AtomBrowserClient : public brightray::BrowserClient {
 public:
  AtomBrowserClient();
  virtual ~AtomBrowserClient();

  // Don't force renderer process to restart for once.
  static void SuppressRendererProcessRestartForOnce();

 protected:
  // content::ContentBrowserClient:
  void RenderProcessWillLaunch(content::RenderProcessHost* host) override;
  content::SpeechRecognitionManagerDelegate*
      CreateSpeechRecognitionManagerDelegate() override;
  content::AccessTokenStore* CreateAccessTokenStore() override;
  void ResourceDispatcherHostCreated() override;
  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                           content::WebPreferences* prefs) override;
  std::string GetApplicationLocale() override;
  void OverrideSiteInstanceForNavigation(
      content::BrowserContext* browser_context,
      content::SiteInstance* current_instance,
      const GURL& dest_url,
      content::SiteInstance** new_instance);
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  content::QuotaPermissionContext* CreateQuotaPermissionContext() override;

 private:
  brightray::BrowserMainParts* OverrideCreateBrowserMainParts(
      const content::MainFunctionParams&) override;

  scoped_ptr<AtomResourceDispatcherHostDelegate> resource_dispatcher_delegate_;

  // The render process which would be swapped out soon.
  content::RenderProcessHost* dying_render_process_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
