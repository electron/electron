// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_
#define BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "content/public/browser/devtools_http_handler_delegate.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace content {
class DevToolsHttpHandler;
}

namespace brightray {

class DevToolsManagerDelegate : public content::DevToolsManagerDelegate {
 public:
  static content::DevToolsHttpHandler* CreateHttpHandler();

  DevToolsManagerDelegate();
  virtual ~DevToolsManagerDelegate();

  // DevToolsManagerDelegate implementation.
  void Inspect(content::BrowserContext* browser_context,
               content::DevToolsAgentHost* agent_host) override {}
  void DevToolsAgentStateChanged(content::DevToolsAgentHost* agent_host,
                                         bool attached) override {}
  base::DictionaryValue* HandleCommand(content::DevToolsAgentHost* agent_host,
                                       base::DictionaryValue* command) override;
  scoped_ptr<content::DevToolsTarget> CreateNewTarget(const GURL& url) override;
  void EnumerateTargets(TargetCallback callback) override;
  std::string GetPageThumbnailData(const GURL& url) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsManagerDelegate);
};

}  // namespace brightray

#endif  // BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_
