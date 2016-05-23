// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_

#include "browser/inspectable_web_contents.h"

#include "browser/devtools_contents_resizing_strategy.h"
#include "browser/devtools_embedder_message_dispatcher.h"

#include "base/memory/weak_ptr.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "ui/gfx/geometry/rect.h"

class PrefService;
class PrefRegistrySimple;

namespace content {
class DevToolsAgentHost;
}

namespace brightray {

class InspectableWebContentsDelegate;
class InspectableWebContentsView;

class InspectableWebContentsImpl :
    public InspectableWebContents,
    public content::DevToolsAgentHostClient,
    public content::WebContentsObserver,
    public content::WebContentsDelegate,
    public DevToolsEmbedderMessageDispatcher::Delegate,
    public net::URLFetcherDelegate {
 public:
  static void RegisterPrefs(PrefRegistrySimple* pref_registry);

  explicit InspectableWebContentsImpl(content::WebContents*);
  virtual ~InspectableWebContentsImpl();

  InspectableWebContentsView* GetView() const override;
  content::WebContents* GetWebContents() const override;
  content::WebContents* GetDevToolsWebContents() const override;

  void SetDelegate(InspectableWebContentsDelegate* delegate) override;
  InspectableWebContentsDelegate* GetDelegate() const override;
  void SetDockState(const std::string& state) override;
  void ShowDevTools() override;
  void CloseDevTools() override;
  bool IsDevToolsViewShowing() override;
  void AttachTo(const scoped_refptr<content::DevToolsAgentHost>&) override;
  void Detach() override;
  void CallClientFunction(const std::string& function_name,
                          const base::Value* arg1,
                          const base::Value* arg2,
                          const base::Value* arg3) override;

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
  void ZoomIn() override;
  void ZoomOut() override;
  void ResetZoom() override;
  void SetDevicesUpdatesEnabled(bool enabled) override;
  void DispatchProtocolMessageFromDevToolsFrontend(
      const std::string& message) override;
  void RecordActionUMA(const std::string& name, int action) override;
  void SendJsonRequest(const DispatchCallback& callback,
                       const std::string& browser_id,
                       const std::string& url) override;
  void GetPreferences(const DispatchCallback& callback) override;
  void SetPreference(const std::string& name,
                     const std::string& value) override;
  void RemovePreference(const std::string& name) override;
  void ClearPreferences() override;

  // content::DevToolsFrontendHostDelegate:
  void HandleMessageFromDevToolsFrontend(const std::string& message);

  // content::DevToolsAgentHostClient:
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               const std::string& message) override;
  void AgentHostClosed(content::DevToolsAgentHost* agent_host,
                       bool replaced) override;

  // content::WebContentsObserver:
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void WebContentsDestroyed() override;
  void OnWebContentsFocused() override;
  void DidStartNavigationToPendingEntry(
      const GURL& url,
      content::NavigationController::ReloadType reload_type) override;

  // content::WebContentsDelegate:
  bool AddMessageToConsole(content::WebContents* source,
                           int32_t level,
                           const base::string16& message,
                           int32_t line_no,
                           const base::string16& source_id) override;
  bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      int32_t route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      WindowContainerType window_container_type,
      const std::string& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;
  void HandleKeyboardEvent(
      content::WebContents*, const content::NativeWebKeyboardEvent&) override;
  void CloseContents(content::WebContents* source) override;

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  void SendMessageAck(int request_id,
                      const base::Value* arg1);

  bool frontend_loaded_;
  scoped_refptr<content::DevToolsAgentHost> agent_host_;
  std::unique_ptr<content::DevToolsFrontendHost> frontend_host_;
  std::unique_ptr<DevToolsEmbedderMessageDispatcher> embedder_message_dispatcher_;

  DevToolsContentsResizingStrategy contents_resizing_strategy_;
  gfx::Rect devtools_bounds_;
  bool can_dock_;
  std::string dock_state_;

  using PendingRequestsMap = std::map<const net::URLFetcher*, DispatchCallback>;
  PendingRequestsMap pending_requests_;
  InspectableWebContentsDelegate* delegate_;  // weak references.

  PrefService* pref_service_;  // weak reference.

  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<content::WebContents> devtools_web_contents_;
  std::unique_ptr<InspectableWebContentsView> view_;

  base::WeakPtrFactory<InspectableWebContentsImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsImpl);
};

}  // namespace brightray

#endif
