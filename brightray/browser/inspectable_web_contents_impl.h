// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brightray/browser/devtools_contents_resizing_strategy.h"
#include "brightray/browser/devtools_embedder_message_dispatcher.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "ui/gfx/geometry/rect.h"

class PrefService;
class PrefRegistrySimple;

namespace brightray {

class InspectableWebContentsDelegate;
class InspectableWebContentsView;

class InspectableWebContentsImpl
    : public InspectableWebContents,
      public content::DevToolsAgentHostClient,
      public content::WebContentsObserver,
      public content::WebContentsDelegate,
      public DevToolsEmbedderMessageDispatcher::Delegate,
      public net::URLFetcherDelegate {
 public:
  static void RegisterPrefs(PrefRegistrySimple* pref_registry);

  InspectableWebContentsImpl(content::WebContents* web_contents, bool is_guest);
  ~InspectableWebContentsImpl() override;

  InspectableWebContentsView* GetView() const override;
  content::WebContents* GetWebContents() const override;
  content::WebContents* GetDevToolsWebContents() const override;

  void SetDelegate(InspectableWebContentsDelegate* delegate) override;
  InspectableWebContentsDelegate* GetDelegate() const override;
  bool IsGuest() const override;
  void ReleaseWebContents() override;
  void SetDevToolsWebContents(content::WebContents* devtools) override;
  void SetDockState(const std::string& state) override;
  void ShowDevTools() override;
  void CloseDevTools() override;
  bool IsDevToolsViewShowing() override;
  void AttachTo(scoped_refptr<content::DevToolsAgentHost>) override;
  void Detach() override;
  void CallClientFunction(const std::string& function_name,
                          const base::Value* arg1,
                          const base::Value* arg2,
                          const base::Value* arg3) override;
  void InspectElement(int x, int y) override;

  // Return the last position and size of devtools window.
  gfx::Rect GetDevToolsBounds() const;
  void SaveDevToolsBounds(const gfx::Rect& bounds);

  // Return the last set zoom level of devtools window.
  double GetDevToolsZoomLevel() const;
  void UpdateDevToolsZoomLevel(double level);

 private:
  // DevToolsEmbedderMessageDispacher::Delegate
  void ActivateWindow() override;
  void CloseWindow() override;
  void LoadCompleted() override;
  void SetInspectedPageBounds(const gfx::Rect& rect) override;
  void InspectElementCompleted() override;
  void InspectedURLChanged(const std::string& url) override;
  void LoadNetworkResource(const DispatchCallback& callback,
                           const std::string& url,
                           const std::string& headers,
                           int stream_id) override;
  void SetIsDocked(const DispatchCallback& callback, bool is_docked) override;
  void OpenInNewTab(const std::string& url) override;
  void ShowItemInFolder(const std::string& file_system_path) override;
  void SaveToFile(const std::string& url,
                  const std::string& content,
                  bool save_as) override;
  void AppendToFile(const std::string& url,
                    const std::string& content) override;
  void RequestFileSystems() override;
  void AddFileSystem(const std::string& file_system_path) override;
  void RemoveFileSystem(const std::string& file_system_path) override;
  void UpgradeDraggedFileSystemPermissions(
      const std::string& file_system_url) override;
  void IndexPath(int index_request_id,
                 const std::string& file_system_path) override;
  void StopIndexing(int index_request_id) override;
  void SearchInPath(int search_request_id,
                    const std::string& file_system_path,
                    const std::string& query) override;
  void SetWhitelistedShortcuts(const std::string& message) override;
  void SetEyeDropperActive(bool active) override;
  void ShowCertificateViewer(const std::string& cert_chain) override;
  void ZoomIn() override;
  void ZoomOut() override;
  void ResetZoom() override;
  void SetDevicesDiscoveryConfig(
      bool discover_usb_devices,
      bool port_forwarding_enabled,
      const std::string& port_forwarding_config,
      bool network_discovery_enabled,
      const std::string& network_discovery_config) override;
  void SetDevicesUpdatesEnabled(bool enabled) override;
  void PerformActionOnRemotePage(const std::string& page_id,
                                 const std::string& action) override;
  void OpenRemotePage(const std::string& browser_id,
                      const std::string& url) override;
  void OpenNodeFrontend() override;
  void DispatchProtocolMessageFromDevToolsFrontend(
      const std::string& message) override;
  void SendJsonRequest(const DispatchCallback& callback,
                       const std::string& browser_id,
                       const std::string& url) override;
  void GetPreferences(const DispatchCallback& callback) override;
  void SetPreference(const std::string& name,
                     const std::string& value) override;
  void RemovePreference(const std::string& name) override;
  void ClearPreferences() override;
  void ConnectionReady() override;
  void RegisterExtensionsAPI(const std::string& origin,
                             const std::string& script) override;

  // content::DevToolsFrontendHostDelegate:
  void HandleMessageFromDevToolsFrontend(const std::string& message);

  // content::DevToolsAgentHostClient:
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               const std::string& message) override;
  void AgentHostClosed(content::DevToolsAgentHost* agent_host) override;

  // content::WebContentsObserver:
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void WebContentsDestroyed() override;
  void OnWebContentsFocused(
      content::RenderWidgetHost* render_widget_host) override;
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // content::WebContentsDelegate:
  bool DidAddMessageToConsole(content::WebContents* source,
                              int32_t level,
                              const base::string16& message,
                              int32_t line_no,
                              const base::string16& source_id) override;
  bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      int32_t route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;
  void HandleKeyboardEvent(content::WebContents*,
                           const content::NativeWebKeyboardEvent&) override;
  void CloseContents(content::WebContents* source) override;
  content::ColorChooser* OpenColorChooser(
      content::WebContents* source,
      SkColor color,
      const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions)
      override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      const content::FileChooserParams& params) override;
  void EnumerateDirectory(content::WebContents* source,
                          int request_id,
                          const base::FilePath& path) override;

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  void SendMessageAck(int request_id, const base::Value* arg1);

  bool frontend_loaded_;
  scoped_refptr<content::DevToolsAgentHost> agent_host_;
  std::unique_ptr<content::DevToolsFrontendHost> frontend_host_;
  std::unique_ptr<DevToolsEmbedderMessageDispatcher>
      embedder_message_dispatcher_;

  DevToolsContentsResizingStrategy contents_resizing_strategy_;
  gfx::Rect devtools_bounds_;
  bool can_dock_;
  std::string dock_state_;

  using PendingRequestsMap = std::map<const net::URLFetcher*, DispatchCallback>;
  PendingRequestsMap pending_requests_;
  InspectableWebContentsDelegate* delegate_;  // weak references.

  PrefService* pref_service_;  // weak reference.

  std::unique_ptr<content::WebContents> web_contents_;

  // The default devtools created by this class when we don't have an external
  // one assigned by SetDevToolsWebContents.
  std::unique_ptr<content::WebContents> managed_devtools_web_contents_;
  // The external devtools assigned by SetDevToolsWebContents.
  content::WebContents* external_devtools_web_contents_ = nullptr;

  bool is_guest_;
  std::unique_ptr<InspectableWebContentsView> view_;

  using ExtensionsAPIs = std::map<std::string, std::string>;
  ExtensionsAPIs extensions_api_;

  base::WeakPtrFactory<InspectableWebContentsImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsImpl);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_
