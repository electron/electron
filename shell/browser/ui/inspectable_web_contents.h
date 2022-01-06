// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ELECTRON_SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_H_
#define ELECTRON_SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_H_

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/devtools/devtools_contents_resizing_strategy.h"
#include "chrome/browser/devtools/devtools_embedder_message_dispatcher.h"
#include "chrome/browser/devtools/devtools_settings.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "electron/buildflags/buildflags.h"
#include "ui/gfx/geometry/rect.h"

class PrefService;
class PrefRegistrySimple;
struct RegisterOptions;

namespace electron {

class InspectableWebContentsDelegate;
class InspectableWebContentsView;

class InspectableWebContents
    : public content::DevToolsAgentHostClient,
      public content::WebContentsObserver,
      public content::WebContentsDelegate,
      public DevToolsEmbedderMessageDispatcher::Delegate {
 public:
  using List = std::list<InspectableWebContents*>;

  static const List& GetAll();
  static void RegisterPrefs(PrefRegistrySimple* pref_registry);

  InspectableWebContents(std::unique_ptr<content::WebContents> web_contents,
                         PrefService* pref_service,
                         bool is_guest);
  ~InspectableWebContents() override;

  // disable copy
  InspectableWebContents(const InspectableWebContents&) = delete;
  InspectableWebContents& operator=(const InspectableWebContents&) = delete;

  InspectableWebContentsView* GetView() const;
  content::WebContents* GetWebContents() const;
  content::WebContents* GetDevToolsWebContents() const;

  void SetDelegate(InspectableWebContentsDelegate* delegate);
  InspectableWebContentsDelegate* GetDelegate() const;
  bool IsGuest() const;
  void ReleaseWebContents();
  void SetDevToolsWebContents(content::WebContents* devtools);
  void SetDockState(const std::string& state);
  void ShowDevTools(bool activate);
  void CloseDevTools();
  bool IsDevToolsViewShowing();
  void AttachTo(scoped_refptr<content::DevToolsAgentHost>);
  void Detach();
  void CallClientFunction(const std::string& function_name,
                          const base::Value* arg1,
                          const base::Value* arg2,
                          const base::Value* arg3);
  void InspectElement(int x, int y);

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
  void LoadNetworkResource(DispatchCallback callback,
                           const std::string& url,
                           const std::string& headers,
                           int stream_id) override;
  void SetIsDocked(DispatchCallback callback, bool is_docked) override;
  void OpenInNewTab(const std::string& url) override;
  void ShowItemInFolder(const std::string& file_system_path) override;
  void SaveToFile(const std::string& url,
                  const std::string& content,
                  bool save_as) override;
  void AppendToFile(const std::string& url,
                    const std::string& content) override;
  void RequestFileSystems() override;
  void AddFileSystem(const std::string& type) override;
  void RemoveFileSystem(const std::string& file_system_path) override;
  void UpgradeDraggedFileSystemPermissions(
      const std::string& file_system_url) override;
  void IndexPath(int index_request_id,
                 const std::string& file_system_path,
                 const std::string& excluded_folders) override;
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
  void SendJsonRequest(DispatchCallback callback,
                       const std::string& browser_id,
                       const std::string& url) override;
  void RegisterPreference(const std::string& name,
                          const RegisterOptions& options) override;
  void GetPreferences(DispatchCallback callback) override;
  void SetPreference(const std::string& name,
                     const std::string& value) override;
  void RemovePreference(const std::string& name) override;
  void ClearPreferences() override;
  void GetSyncInformation(DispatchCallback callback) override;
  void ConnectionReady() override;
  void RegisterExtensionsAPI(const std::string& origin,
                             const std::string& script) override;
  void Reattach(DispatchCallback callback) override;
  void RecordEnumeratedHistogram(const std::string& name,
                                 int sample,
                                 int boundary_value) override {}
  void ReadyForTest() override {}
  void SetOpenNewWindowForPopups(bool value) override {}
  void RecordPerformanceHistogram(const std::string& name,
                                  double duration) override {}
  void RecordUserMetricsAction(const std::string& name) override {}
  void ShowSurvey(DispatchCallback callback,
                  const std::string& trigger) override {}
  void CanShowSurvey(DispatchCallback callback,
                     const std::string& trigger) override {}

  // content::DevToolsFrontendHostDelegate:
  void HandleMessageFromDevToolsFrontend(base::Value message);

  // content::DevToolsAgentHostClient:
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               base::span<const uint8_t> message) override;
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
  bool HandleKeyboardEvent(content::WebContents*,
                           const content::NativeWebKeyboardEvent&) override;
  void CloseContents(content::WebContents* source) override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      scoped_refptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override;
  void EnumerateDirectory(content::WebContents* source,
                          scoped_refptr<content::FileSelectListener> listener,
                          const base::FilePath& path) override;

  void SendMessageAck(int request_id, const base::Value* arg1);

  const char* GetDictionaryNameForSettingsName(const std::string& name) const;
  const char* GetDictionaryNameForSyncedPrefs() const;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  void AddDevToolsExtensionsToClient();
#endif

  DevToolsContentsResizingStrategy contents_resizing_strategy_;
  gfx::Rect devtools_bounds_;
  bool can_dock_ = true;
  std::string dock_state_;
  bool activate_ = true;

  InspectableWebContentsDelegate* delegate_ = nullptr;  // weak references.

  PrefService* pref_service_;  // weak reference.

  std::unique_ptr<content::WebContents> web_contents_;

  // The default devtools created by this class when we don't have an external
  // one assigned by SetDevToolsWebContents.
  std::unique_ptr<content::WebContents> managed_devtools_web_contents_;
  // The external devtools assigned by SetDevToolsWebContents.
  content::WebContents* external_devtools_web_contents_ = nullptr;

  bool is_guest_;
  std::unique_ptr<InspectableWebContentsView> view_;

  bool frontend_loaded_ = false;
  scoped_refptr<content::DevToolsAgentHost> agent_host_;
  std::unique_ptr<content::DevToolsFrontendHost> frontend_host_;
  std::unique_ptr<DevToolsEmbedderMessageDispatcher>
      embedder_message_dispatcher_;

  class NetworkResourceLoader;
  std::set<std::unique_ptr<NetworkResourceLoader>, base::UniquePtrComparator>
      loaders_;

  using ExtensionsAPIs = std::map<std::string, std::string>;
  ExtensionsAPIs extensions_api_;

  // Contains the set of synced settings.
  // The DevTools frontend *must* call `Register` for each setting prior to
  // use, which guarantees that this set must not be persisted.
  base::flat_set<std::string> synced_setting_names_;

  base::WeakPtrFactory<InspectableWebContents> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_H_
