// Copyright (c) 2024 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_UI_THEME_DATA_SOURCE_H_
#define ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_UI_THEME_DATA_SOURCE_H_

#include "content/public/browser/url_data_source.h"

namespace electron {
// A ThemeDataSource implementation that handles devtools://theme/
// requests.
class ThemeDataSource : public content::URLDataSource {
 public:
  ThemeDataSource() = default;
  ~ThemeDataSource() override = default;

  ThemeDataSource(const ThemeDataSource&) = delete;
  ThemeDataSource& operator=(const ThemeDataSource&) = delete;

  std::string GetSource() override;

  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;

 private:
  std::string GetMimeType(const GURL& url) override;

  void SendColorsCss(const GURL& url,
                     const content::WebContents::Getter& wc_getter,
                     content::URLDataSource::GotDataCallback callback);
};

}  // namespace electron
#endif  // ELECTRON_SHELL_BROWSER_UI_DEVTOOLS_UI_THEME_DATA_SOURCE_H_
