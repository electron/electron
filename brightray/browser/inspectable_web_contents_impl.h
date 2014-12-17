// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_

#include "browser/inspectable_web_contents.h"

#include "browser/devtools_contents_resizing_strategy.h"
#include "browser/devtools_embedder_message_dispatcher.h"

#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/gfx/rect.h"

class PrefRegistrySimple;

namespace content {
class DevToolsAgentHost;
}

namespace brightray {

class InspectableWebContentsDelegate;
class InspectableWebContentsView;

class InspectableWebContentsImpl :
    public InspectableWebContents,
    public content::DevToolsFrontendHost::Delegate,
    public content::DevToolsAgentHostClient,
    public content::WebContentsObserver,
    public content::WebContentsDelegate,
    public DevToolsEmbedderMessageDispatcher::Delegate {
 public:
  static void RegisterPrefs(PrefRegistrySimple* pref_registry);

  explicit InspectableWebContentsImpl(content::WebContents*);
  virtual ~InspectableWebContentsImpl();

  InspectableWebContentsView* GetView() const override;
  content::WebContents* GetWebContents() const override;

  void SetCanDock(bool can_dock) override;
  void ShowDevTools() override;
  void CloseDevTools() override;
  bool IsDevToolsViewShowing() override;

  // Return the last position and size of devtools window.
  gfx::Rect GetDevToolsBounds() const;
  void SaveDevToolsBounds(const gfx::Rect& bounds);

  virtual void SetDelegate(InspectableWebContentsDelegate* delegate) {
    delegate_ = delegate;
  }
  virtual InspectableWebContentsDelegate* GetDelegate() const {
    return delegate_;
  }

  content::WebContents* devtools_web_contents() {
    return devtools_web_contents_.get();
  }

 private:
  // DevToolsEmbedderMessageDispacher::Delegate
  void ActivateWindow() override;
  void CloseWindow() override;
  void SetInspectedPageBounds(const gfx::Rect& rect) override;
  void InspectElementCompleted() override;
  void MoveWindow(int x, int y) override;
  void SetIsDocked(bool docked) override;
  void OpenInNewTab(const std::string& url) override;
  void SaveToFile(const std::string& url,
                  const std::string& content,
                  bool save_as) override;
  void AppendToFile(const std::string& url,
                    const std::string& content) override;
  void RequestFileSystems() override;
  void AddFileSystem() override;
  void RemoveFileSystem(const std::string& file_system_path) override;
  void UpgradeDraggedFileSystemPermissions(
      const std::string& file_system_url) override;
  void IndexPath(int request_id,
                 const std::string& file_system_path) override;
  void StopIndexing(int request_id) override;
  void SearchInPath(int request_id,
                    const std::string& file_system_path,
                    const std::string& query) override;
  void ZoomIn() override;
  void ZoomOut() override;
  void ResetZoom() override;

  // content::DevToolsFrontendHostDelegate:
  void HandleMessageFromDevToolsFrontend(const std::string& message) override;
  void HandleMessageFromDevToolsFrontendToBackend(const std::string& message) override;

  // content::DevToolsAgentHostClient:
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               const std::string& message) override;
  void AgentHostClosed(content::DevToolsAgentHost* agent_host,
                       bool replaced) override;

  // content::WebContentsObserver:
  void AboutToNavigateRenderView(content::RenderViewHost* render_view_host) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void WebContentsDestroyed() override;

  // content::WebContentsDelegate
  bool AddMessageToConsole(content::WebContents* source,
                           int32 level,
                           const base::string16& message,
                           int32 line_no,
                           const base::string16& source_id) override;
  bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      int route_id,
      WindowContainerType window_container_type,
      const base::string16& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;
  void HandleKeyboardEvent(
      content::WebContents*, const content::NativeWebKeyboardEvent&) override;
  void CloseContents(content::WebContents* source) override;

  scoped_ptr<content::WebContents> web_contents_;
  scoped_ptr<content::WebContents> devtools_web_contents_;
  scoped_ptr<InspectableWebContentsView> view_;
  scoped_refptr<content::DevToolsAgentHost> agent_host_;
  scoped_ptr<content::DevToolsFrontendHost> frontend_host_;

  DevToolsContentsResizingStrategy contents_resizing_strategy_;
  gfx::Rect devtools_bounds_;
  bool can_dock_;

  scoped_ptr<DevToolsEmbedderMessageDispatcher> embedder_message_dispatcher_;

  InspectableWebContentsDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsImpl);
};

}  // namespace brightray

#endif
