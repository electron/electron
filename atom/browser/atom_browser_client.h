// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_

#include <map>
#include <set>
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

  using Delegate = content::ContentBrowserClient;
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // Returns the WebContents for pending render processes.
  content::WebContents* GetWebContentsFromProcessID(int process_id);

  // Don't force renderer process to restart for once.
  static void SuppressRendererProcessRestartForOnce();

  // Custom schemes to be registered to handle service worker.
  static void SetCustomServiceWorkerSchemes(
      const std::vector<std::string>& schemes);

 protected:
  // content::ContentBrowserClient:
  void RenderProcessWillLaunch(content::RenderProcessHost* host) override;
  content::SpeechRecognitionManagerDelegate*
      CreateSpeechRecognitionManagerDelegate() override;
  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                           content::WebPreferences* prefs) override;
  std::string GetApplicationLocale() override;
  void OverrideSiteInstanceForNavigation(
      content::RenderFrameHost* render_frame_host,
      content::BrowserContext* browser_context,
      content::SiteInstance* current_instance,
      const GURL& dest_url,
      content::SiteInstance** new_instance) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  content::QuotaPermissionContext* CreateQuotaPermissionContext() override;
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool overridable,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(content::CertificateRequestResultType)>&
          callback) override;
  void SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      std::unique_ptr<content::ClientCertificateDelegate> delegate) override;
  void ResourceDispatcherHostCreated() override;
  bool CanCreateWindow(
      int opener_render_process_id,
      int opener_render_frame_id,
      const GURL& opener_url,
      const GURL& opener_top_level_frame_url,
      const GURL& source_origin,
      content::mojom::WindowContainerType container_type,
      const GURL& target_url,
      const content::Referrer& referrer,
      const std::string& frame_name,
      WindowOpenDisposition disposition,
      const blink::mojom::WindowFeatures& features,
      const std::vector<std::string>& additional_features,
      const scoped_refptr<content::ResourceRequestBodyImpl>& body,
      bool user_gesture,
      bool opener_suppressed,
      content::ResourceContext* context,
      bool* no_javascript_access) override;
  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* schemes) override;

  // brightray::BrowserClient:
  brightray::BrowserMainParts* OverrideCreateBrowserMainParts(
      const content::MainFunctionParams&) override;
  void WebNotificationAllowed(
      int render_process_id,
      const base::Callback<void(bool, bool)>& callback) override;

  // content::RenderProcessHostObserver:
  void RenderProcessHostDestroyed(content::RenderProcessHost* host) override;
  void RenderProcessReady(content::RenderProcessHost* host) override;
  void RenderProcessExited(content::RenderProcessHost* host,
                           base::TerminationStatus status,
                           int exit_code) override;

 private:
  bool ShouldCreateNewSiteInstance(content::RenderFrameHost* render_frame_host,
                                   content::BrowserContext* browser_context,
                                   content::SiteInstance* current_instance,
                                   const GURL& dest_url);
  struct ProcessPreferences {
    bool sandbox;
    bool native_window_open;
    bool disable_popups;
  };
  void AddProcessPreferences(int process_id, ProcessPreferences prefs);
  void RemoveProcessPreferences(int process_id);
  bool IsRendererSandboxed(int process_id);
  bool RendererUsesNativeWindowOpen(int process_id);
  bool RendererDisablesPopups(int process_id);

  // pending_render_process => current_render_process.
  std::map<int, int> pending_processes_;

  std::map<int, ProcessPreferences> process_preferences_;
  std::map<int, base::ProcessId> render_process_host_pids_;
  base::Lock process_preferences_lock_;

  std::unique_ptr<AtomResourceDispatcherHostDelegate>
      resource_dispatcher_host_delegate_;

  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
