// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_DEVTOOLS_DELEGATE_H_
#define BRIGHTRAY_BROWSER_DEVTOOLS_DELEGATE_H_

#include "content/public/browser/devtools_http_handler_delegate.h"

namespace brightray {

class DevToolsDelegate : public content::DevToolsHttpHandlerDelegate {
public:
  DevToolsDelegate();
  ~DevToolsDelegate();

private:
  virtual std::string GetDiscoveryPageHTML() OVERRIDE;
  virtual bool BundlesFrontendResources() OVERRIDE;
  virtual base::FilePath GetDebugFrontendDir() OVERRIDE;
  virtual std::string GetPageThumbnailData(const GURL&) OVERRIDE;
  virtual content::RenderViewHost* CreateNewTarget() OVERRIDE;
  virtual TargetType GetTargetType(content::RenderViewHost*) OVERRIDE;
  virtual std::string GetViewDescription(content::RenderViewHost*) OVERRIDE;
  virtual scoped_refptr<net::StreamListenSocket> CreateSocketForTethering(
      net::StreamListenSocket::Delegate*,
      std::string* name) OVERRIDE;
};

}

#endif
