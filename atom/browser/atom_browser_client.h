// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_

#include <string>
#include <vector>

#include "brightray/browser/browser_client.h"

namespace content {
class QuotaPermissionContext;
class ClientCertificateDelegate;
}

namespace net {
class SSLCertRequestInfo;
}

namespace atom {

class AtomBrowserClient : public brightray::BrowserClient {
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
      content::SiteInstance** new_instance);
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  content::QuotaPermissionContext* CreateQuotaPermissionContext() override;
  void SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      scoped_ptr<content::ClientCertificateDelegate> delegate) override;

 private:
  brightray::BrowserMainParts* OverrideCreateBrowserMainParts(
      const content::MainFunctionParams&) override;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
