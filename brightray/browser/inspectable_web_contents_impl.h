// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_

#include "browser/inspectable_web_contents.h"

#include "content/public/browser/devtools_frontend_host_delegate.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"

class PrefRegistrySimple;

namespace content {
class DevToolsAgentHost;
class DevToolsClientHost;
}

namespace brightray {

class InspectableWebContentsView;

class InspectableWebContentsImpl :
    public InspectableWebContents,
    content::DevToolsFrontendHostDelegate,
    content::WebContentsObserver,
    content::WebContentsDelegate {
public:
  static void RegisterPrefs(PrefRegistrySimple*);

  InspectableWebContentsImpl(content::WebContents*);
  virtual ~InspectableWebContentsImpl() OVERRIDE;

  virtual InspectableWebContentsView* GetView() const OVERRIDE;
  virtual content::WebContents* GetWebContents() const OVERRIDE;

  virtual void ShowDevTools() OVERRIDE;

  content::WebContents* devtools_web_contents() { return devtools_web_contents_.get(); }

private:
  void UpdateFrontendDockSide();

  // content::DevToolsFrontendHostDelegate
  
  virtual void ActivateWindow() OVERRIDE;
  virtual void ChangeAttachedWindowHeight(unsigned height) OVERRIDE;
  virtual void CloseWindow() OVERRIDE;
  virtual void MoveWindow(int x, int y) OVERRIDE;
  virtual void SetDockSide(const std::string& side) OVERRIDE;
  virtual void OpenInNewTab(const std::string& url) OVERRIDE;
  virtual void SaveToFile(const std::string& url,
                          const std::string& content,
                          bool save_as) OVERRIDE;
  virtual void AppendToFile(const std::string& url,
                            const std::string& content) OVERRIDE;
  virtual void RequestFileSystems() OVERRIDE;
  virtual void AddFileSystem() OVERRIDE;
  virtual void RemoveFileSystem(const std::string& file_system_path) OVERRIDE;
  virtual void InspectedContentsClosing() OVERRIDE;
  
  // content::WebContentsObserver
  
  virtual void RenderViewCreated(content::RenderViewHost*) OVERRIDE;
  virtual void DidFinishLoad(int64 frame_id,
                             const GURL& validated_url,
                             bool is_main_frame,
                             content::RenderViewHost*) OVERRIDE;
  virtual void WebContentsDestroyed(content::WebContents*) OVERRIDE;

  // content::WebContentsDelegate
  
  virtual void HandleKeyboardEvent(content::WebContents*, const content::NativeWebKeyboardEvent&) OVERRIDE;
  
  scoped_ptr<content::WebContents> web_contents_;
  scoped_ptr<content::WebContents> devtools_web_contents_;
  scoped_ptr<InspectableWebContentsView> view_;
  scoped_refptr<content::DevToolsAgentHost> agent_host_;
  scoped_ptr<content::DevToolsClientHost> frontend_host_;
  std::string dock_side_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsImpl);
};

}

#endif
