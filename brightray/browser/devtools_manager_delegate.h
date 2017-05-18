// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_
#define BRIGHTRAY_BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace brightray {

class DevToolsNetworkProtocolHandler;

class DevToolsManagerDelegate : public content::DevToolsManagerDelegate {
 public:
  static void StartHttpHandler();

  DevToolsManagerDelegate();
  virtual ~DevToolsManagerDelegate();

  // DevToolsManagerDelegate implementation.
  void Inspect(content::DevToolsAgentHost* agent_host) override;
  base::DictionaryValue* HandleCommand(
      content::DevToolsAgentHost* agent_host,
      base::DictionaryValue* command) override;
  scoped_refptr<content::DevToolsAgentHost> CreateNewTarget(
      const GURL& url) override;
  std::string GetDiscoveryPageHTML() override;
  std::string GetFrontendResource(const std::string& path) override;

 private:
  std::unique_ptr<DevToolsNetworkProtocolHandler> handler_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsManagerDelegate);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_DEVTOOLS_MANAGER_DELEGATE_H_
