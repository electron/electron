// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/devtools_ui.h"

#include <string>

#include "browser/browser_context.h"

#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"

using content::WebContents;

namespace brightray {

namespace {

const char kChromeUIDevToolsBundledHost[] = "devtools";

std::string PathWithoutParams(const std::string& path) {
  return GURL(std::string("chrome-devtools://devtools/") + path)
      .path().substr(1);
}

std::string GetMimeTypeForPath(const std::string& path) {
  std::string filename = PathWithoutParams(path);
  if (EndsWith(filename, ".html", false)) {
    return "text/html";
  } else if (EndsWith(filename, ".css", false)) {
    return "text/css";
  } else if (EndsWith(filename, ".js", false)) {
    return "application/javascript";
  } else if (EndsWith(filename, ".png", false)) {
    return "image/png";
  } else if (EndsWith(filename, ".gif", false)) {
    return "image/gif";
  } else if (EndsWith(filename, ".svg", false)) {
    return "image/svg+xml";
  } else if (EndsWith(filename, ".manifest", false)) {
    return "text/cache-manifest";
  }
  return "text/html";
}

class BundledDataSource : public content::URLDataSource {
 public:
  BundledDataSource() {}

  // content::URLDataSource implementation.
  std::string GetSource() const override {
    return kChromeUIDevToolsBundledHost;
  }

  void StartDataRequest(const std::string& path,
                        int render_process_id,
                        int render_view_id,
                        const GotDataCallback& callback) override {
    std::string filename = PathWithoutParams(path);
    base::StringPiece resource =
      content::DevToolsFrontendHost::GetFrontendResource(filename);
    scoped_refptr<base::RefCountedStaticMemory> bytes(
        new base::RefCountedStaticMemory(resource.data(), resource.length()));
    callback.Run(bytes.get());
  }

  std::string GetMimeType(const std::string& path) const override {
    return GetMimeTypeForPath(path);
  }

  bool ShouldAddContentSecurityPolicy() const override {
    return false;
  }

  bool ShouldDenyXFrameOptions() const override {
    return false;
  }

  bool ShouldServeMimeTypeAsContentTypeHeader() const override {
    return true;
  }

 private:
  virtual ~BundledDataSource() {}
  DISALLOW_COPY_AND_ASSIGN(BundledDataSource);
};

}  // namespace

DevToolsUI::DevToolsUI(BrowserContext* browser_context, content::WebUI* web_ui)
    : WebUIController(web_ui) {
  web_ui->SetBindings(0);
  content::URLDataSource::Add(browser_context, new BundledDataSource());
}

}  // namespace brightray
