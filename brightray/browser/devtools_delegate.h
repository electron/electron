// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_DEVTOOLS_DELEGATE_H_
#define BRIGHTRAY_DEVTOOLS_DELEGATE_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "content/public/browser/devtools_http_handler_delegate.h"

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
  virtual std::string GetDiscoveryPageHTML() OVERRIDE;
  virtual bool BundlesFrontendResources() OVERRIDE;
  virtual base::FilePath GetDebugFrontendDir() OVERRIDE;
  virtual std::string GetPageThumbnailData(const GURL& url) OVERRIDE;
  virtual scoped_ptr<content::DevToolsTarget> CreateNewTarget(
      const GURL& url) OVERRIDE;
  virtual void EnumerateTargets(TargetCallback callback) OVERRIDE;
  virtual scoped_ptr<net::StreamListenSocket> CreateSocketForTethering(
      net::StreamListenSocket::Delegate* delegate,
      std::string* name) OVERRIDE;

  content::DevToolsHttpHandler* devtools_http_handler() {
    return devtools_http_handler_;
  }

 private:
  content::BrowserContext* browser_context_;
  content::DevToolsHttpHandler* devtools_http_handler_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsDelegate);
};

}  // namespace brightray

#endif  // BRIGHTRAY_DEVTOOLS_DELEGATE_H_
