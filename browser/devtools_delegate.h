// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_DEVTOOLS_DELEGATE_H_
#define ATOM_BROWSER_DEVTOOLS_DELEGATE_H_

#include "base/memory/scoped_ptr.h"
#include "browser/native_window_observer.h"
#include "content/public/browser/devtools_frontend_host_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "vendor/brightray/browser/devtools_embedder_message_dispatcher.h"
#include "vendor/brightray/browser/inspectable_web_contents_delegate.h"

namespace content {
class DevToolsAgentHost;
class DevToolsClientHost;
}

using brightray::DevToolsEmbedderMessageDispatcher;

namespace atom {

class NativeWindow;

class DevToolsDelegate : public content::DevToolsFrontendHostDelegate,
                         public content::WebContentsObserver,
                         public NativeWindowObserver,
                         public DevToolsEmbedderMessageDispatcher::Delegate {
 public:
  DevToolsDelegate(NativeWindow* window,
                   content::WebContents* target_web_contents);
  virtual ~DevToolsDelegate();

  void SetDelegate(brightray::InspectableWebContentsDelegate* delegate) {
    delegate_ = delegate;
  }

 protected:
  // Implementations of content::DevToolsFrontendHostDelegate.
  virtual void DispatchOnEmbedder(const std::string& message) OVERRIDE;
  virtual void InspectedContentsClosing() OVERRIDE;

  // Implementations of content::WebContentsObserver.
  virtual void AboutToNavigateRenderView(
      content::RenderViewHost* render_view_host) OVERRIDE;

  // Implementations of NativeWindowObserver.
  virtual void OnWindowClosed() OVERRIDE;

  // Implementations of DevToolsEmbedderMessageDispatcher::Delegate.
  virtual void ActivateWindow() OVERRIDE;
  virtual void CloseWindow() OVERRIDE;
  virtual void MoveWindow(int x, int y) OVERRIDE;
  virtual void SetDockSide(const std::string& dock_side) OVERRIDE;
  virtual void OpenInNewTab(const std::string& url) OVERRIDE;
  virtual void SaveToFile(const std::string& url,
                          const std::string& content,
                          bool save_as) OVERRIDE;
  virtual void AppendToFile(const std::string& url,
                            const std::string& content) OVERRIDE;
  virtual void RequestFileSystems() OVERRIDE;
  virtual void AddFileSystem() OVERRIDE;
  virtual void RemoveFileSystem(const std::string& file_system_path) OVERRIDE;
  virtual void IndexPath(int request_id,
                         const std::string& file_system_path) OVERRIDE;
  virtual void StopIndexing(int request_id) OVERRIDE;
  virtual void SearchInPath(int request_id,
                            const std::string& file_system_path,
                            const std::string& query) OVERRIDE;

 private:
  NativeWindow* owner_window_;
  brightray::InspectableWebContentsDelegate* delegate_;

  scoped_refptr<content::DevToolsAgentHost> devtools_agent_host_;
  scoped_ptr<content::DevToolsClientHost> devtools_client_host_;
  scoped_ptr<DevToolsEmbedderMessageDispatcher> embedder_message_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_DEVTOOLS_DELEGATE_H_
