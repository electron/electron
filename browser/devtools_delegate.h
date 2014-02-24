// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_DEVTOOLS_DELEGATE_H_
#define ATOM_BROWSER_DEVTOOLS_DELEGATE_H_

#include "base/memory/scoped_ptr.h"
#include "content/public/browser/devtools_frontend_host_delegate.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class DevToolsAgentHost;
class DevToolsClientHost;
}

namespace atom {

class NativeWindow;

class DevToolsDelegate : public content::DevToolsFrontendHostDelegate,
                         public content::WebContentsObserver {
 public:
  DevToolsDelegate(NativeWindow* window,
                   content::WebContents* target_web_contents);
  virtual ~DevToolsDelegate();

 protected:
  // Implementations of content::DevToolsFrontendHostDelegate.
  virtual void DispatchOnEmbedder(const std::string& message) OVERRIDE;
  virtual void InspectedContentsClosing() OVERRIDE;

  // Implementations of content::WebContentsObserver.
  virtual void AboutToNavigateRenderView(
      content::RenderViewHost* render_view_host) OVERRIDE;

 private:
  NativeWindow* owner_window_;

  scoped_refptr<content::DevToolsAgentHost> devtools_agent_host_;
  scoped_ptr<content::DevToolsClientHost> devtools_client_host_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_DEVTOOLS_DELEGATE_H_
