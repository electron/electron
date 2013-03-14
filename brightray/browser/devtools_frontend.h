// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_DEVTOOLS_FRONTEND_H_
#define BRIGHTRAY_BROWSER_DEVTOOLS_FRONTEND_H_

#include "base/memory/scoped_ptr.h"
#include "content/public/browser/devtools_frontend_host_delegate.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class DevToolsAgentHost;
class DevToolsClientHost;
}

namespace brightray {

class DevToolsFrontend : content::DevToolsFrontendHostDelegate, content::WebContentsObserver {
public:
  static content::WebContents* Show(content::WebContents* inspected_contents);

private:
  DevToolsFrontend(content::WebContents* inspected_contents);
  ~DevToolsFrontend();

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

  virtual void RenderViewCreated(content::RenderViewHost*) OVERRIDE;
  virtual void WebContentsDestroyed(content::WebContents*) OVERRIDE;

  scoped_refptr<content::DevToolsAgentHost> agent_host_;
  scoped_ptr<content::DevToolsClientHost> frontend_host_;
};

}

#endif
