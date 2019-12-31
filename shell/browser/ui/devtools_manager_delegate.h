// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef SHELL_BROWSER_UI_DEVTOOLS_MANAGER_DELEGATE_H_
#define SHELL_BROWSER_UI_DEVTOOLS_MANAGER_DELEGATE_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace electron {

class DevToolsManagerDelegate : public content::DevToolsManagerDelegate {
 public:
  static void StartHttpHandler();

  DevToolsManagerDelegate();
  ~DevToolsManagerDelegate() override;

  // DevToolsManagerDelegate implementation.
  void Inspect(content::DevToolsAgentHost* agent_host) override;
  void HandleCommand(content::DevToolsAgentHost* agent_host,
                     content::DevToolsAgentHostClient* client,
                     const std::string& method,
                     base::span<const uint8_t> message,
                     NotHandledCallback callback) override;
  scoped_refptr<content::DevToolsAgentHost> CreateNewTarget(
      const GURL& url) override;
  std::string GetDiscoveryPageHTML() override;
  bool HasBundledFrontendResources() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsManagerDelegate);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_DEVTOOLS_MANAGER_DELEGATE_H_
