// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_DEVTOOLS_DELEGATE_H_
#define BRIGHTRAY_DEVTOOLS_DELEGATE_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "content/public/browser/devtools_http_handler_delegate.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace content {
class BrowserContext;
class DevToolsHttpHandler;
}

namespace brightray {

class DevToolsDelegate : public content::DevToolsHttpHandlerDelegate {
 public:
  explicit DevToolsDelegate(content::BrowserContext* browser_context);
  virtual ~DevToolsDelegate();

  // Stops http server.
  void Stop();

  // DevToolsHttpProtocolHandler::Delegate overrides.
  std::string GetDiscoveryPageHTML() override;
  bool BundlesFrontendResources() override;
  base::FilePath GetDebugFrontendDir() override;
  scoped_ptr<net::StreamListenSocket> CreateSocketForTethering(
      net::StreamListenSocket::Delegate* delegate,
      std::string* name) override;

  content::DevToolsHttpHandler* devtools_http_handler() {
    return devtools_http_handler_;
  }

 private:
  content::BrowserContext* browser_context_;
  content::DevToolsHttpHandler* devtools_http_handler_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsDelegate);
};

class DevToolsManagerDelegate : public content::DevToolsManagerDelegate {
 public:
  explicit DevToolsManagerDelegate(content::BrowserContext* browser_context);
  virtual ~DevToolsManagerDelegate();

  // DevToolsManagerDelegate implementation.
  void Inspect(content::BrowserContext* browser_context,
               content::DevToolsAgentHost* agent_host) override {}
  void DevToolsAgentStateChanged(content::DevToolsAgentHost* agent_host,
                                         bool attached) override {}
  base::DictionaryValue* HandleCommand(
      content::DevToolsAgentHost* agent_host,
      base::DictionaryValue* command) override;
  scoped_ptr<content::DevToolsTarget> CreateNewTarget(
      const GURL& url) override;
  void EnumerateTargets(TargetCallback callback) override;
  std::string GetPageThumbnailData(const GURL& url) override;

 private:
  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsManagerDelegate);
};

}  // namespace brightray

#endif  // BRIGHTRAY_DEVTOOLS_DELEGATE_H_
