// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_BROWSER_CLIENT_H_
#define BRIGHTRAY_BROWSER_BROWSER_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "content/public/browser/content_browser_client.h"

namespace brightray {

class BrowserMainParts;

class BrowserClient : public content::ContentBrowserClient {
 public:
  static BrowserClient* Get();
  static void SetApplicationLocale(const std::string& locale);

  BrowserClient();
  ~BrowserClient() override;

  BrowserMainParts* browser_main_parts() { return browser_main_parts_; }

  // Subclasses that override this (e.g., to provide their own protocol
  // handlers) should call this implementation after doing their own work.
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams&) override;
  content::MediaObserver* GetMediaObserver() override;
  base::FilePath GetDefaultDownloadDirectory() override;
  std::string GetApplicationLocale() override;

 protected:
  // Subclasses should override this to provide their own BrowserMainParts
  // implementation. The lifetime of the returned instance is managed by the
  // caller.
  virtual BrowserMainParts* OverrideCreateBrowserMainParts(
      const content::MainFunctionParams&);

 private:
  BrowserMainParts* browser_main_parts_;

  DISALLOW_COPY_AND_ASSIGN(BrowserClient);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_BROWSER_CLIENT_H_
