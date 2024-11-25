// Copyright (c) 2024 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_UI_BUNDLE_DATA_SOURCE_H_
#define ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_UI_BUNDLE_DATA_SOURCE_H_

#include "content/public/browser/url_data_source.h"

namespace electron {
// A BundledDataSource implementation that handles devtools://devtools/
// requests.
class BundledDataSource : public content::URLDataSource {
 public:
  BundledDataSource() = default;
  ~BundledDataSource() override = default;

  // disable copy
  BundledDataSource(const BundledDataSource&) = delete;
  BundledDataSource& operator=(const BundledDataSource&) = delete;

  std::string GetSource() override;

  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;

 private:
  std::string GetMimeType(const GURL& url) override;

  bool ShouldAddContentSecurityPolicy() override;
  bool ShouldDenyXFrameOptions() override;
  bool ShouldServeMimeTypeAsContentTypeHeader() override;

  void StartBundledDataRequest(const std::string& path,
                               GotDataCallback callback);
};
}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_UI_BUNDLE_DATA_SOURCE_H_
