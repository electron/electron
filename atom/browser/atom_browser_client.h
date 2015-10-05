// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_

#include <map>
#include <string>
#include <vector>

#include "brightray/browser/browser_client.h"
#include "content/public/browser/render_process_host_observer.h"

namespace content {
class QuotaPermissionContext;
class ClientCertificateDelegate;
}

namespace net {
class SSLCertRequestInfo;
}

namespace atom {

class AtomResourceDispatcherHostDelegate;

class AtomBrowserClient : public brightray::BrowserClient,
                          public content::RenderProcessHostObserver {
 public:
  AtomBrowserClient();
  virtual ~AtomBrowserClient();

  // Don't force renderer process to restart for once.
  static void SuppressRendererProcessRestartForOnce();
  // Custom schemes to be registered to standard.
  static void SetCustomSchemes(const std::vector<std::string>& schemes);

 protected:
  // content::ContentBrowserClient:
  void RenderProcessWillLaunch(content::RenderProcessHost* host) override;
  content::SpeechRecognitionManagerDelegate*
      CreateSpeechRecognitionManagerDelegate() override;
  content::AccessTokenStore* CreateAccessTokenStore() override;
  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                           content::WebPreferences* prefs) override;
  std::string GetApplicationLocale() override;
  void OverrideSiteInstanceForNavigation(
      content::BrowserContext* browser_context,
      content::SiteInstance* current_instance,
      const GURL& dest_url,
      content::SiteInstance** new_instance) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  content::QuotaPermissionContext* CreateQuotaPermissionContext() override;
  void SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      scoped_ptr<content::ClientCertificateDelegate> delegate) override;
  void ResourceDispatcherHostCreated() override;

  // brightray::BrowserClient:
  brightray::BrowserMainParts* OverrideCreateBrowserMainParts(
      const content::MainFunctionParams&) override;

  // content::RenderProcessHostObserver:
  void RenderProcessHostDestroyed(content::RenderProcessHost* host) override;

 private:
  // pending_render_process => current_render_process.
  std::map<int, int> pending_processes_;

  scoped_ptr<AtomResourceDispatcherHostDelegate>
      resource_dispatcher_host_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
