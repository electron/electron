// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/devtools_ui.h"

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"


namespace brightray {

namespace {

const char kChromeUIDevToolsHost[] = "devtools";
const char kChromeUIDevToolsBundledPath[] = "bundled";

std::string PathWithoutParams(const std::string& path) {
  return GURL(std::string("chrome-devtools://devtools/") + path)
      .path().substr(1);
}

std::string GetMimeTypeForPath(const std::string& path) {
  std::string filename = PathWithoutParams(path);
  if (base::EndsWith(filename, ".html", base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/html";
  } else if (base::EndsWith(filename, ".css",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/css";
  } else if (base::EndsWith(filename, ".js",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/javascript";
  } else if (base::EndsWith(filename, ".png",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/png";
  } else if (base::EndsWith(filename, ".gif",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/gif";
  } else if (base::EndsWith(filename, ".svg",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/svg+xml";
  } else if (base::EndsWith(filename, ".manifest",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/cache-manifest";
  }
  return "text/html";
}

class BundledDataSource : public content::URLDataSource {
 public:
  BundledDataSource() {}

  // content::URLDataSource implementation.
  std::string GetSource() const override {
    return kChromeUIDevToolsHost;
  }

  void StartDataRequest(const std::string& path,
                        int render_process_id,
                        int render_frame_id,
                        const GotDataCallback& callback) override {
    // Serve request from local bundle.
    std::string bundled_path_prefix(kChromeUIDevToolsBundledPath);
    bundled_path_prefix += "/";
    if (base::StartsWith(path, bundled_path_prefix,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      StartBundledDataRequest(path.substr(bundled_path_prefix.length()),
                              render_process_id, render_frame_id, callback);
      return;
    }
    callback.Run(nullptr);
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

  void StartBundledDataRequest(
      const std::string& path,
      int render_process_id,
      int render_frame_id,
      const content::URLDataSource::GotDataCallback& callback) {
    std::string filename = PathWithoutParams(path);
    base::StringPiece resource =
        content::DevToolsFrontendHost::GetFrontendResource(filename);

    DLOG_IF(WARNING, resource.empty())
        << "Unable to find dev tool resource: " << filename
        << ". If you compiled with debug_devtools=1, try running with "
           "--debug-devtools.";
    scoped_refptr<base::RefCountedStaticMemory> bytes(
        new base::RefCountedStaticMemory(resource.data(), resource.length()));
    callback.Run(bytes.get());
  }

 private:
  ~BundledDataSource() override {}
  DISALLOW_COPY_AND_ASSIGN(BundledDataSource);
};

}  // namespace

DevToolsUI::DevToolsUI(content::BrowserContext* browser_context,
                       content::WebUI* web_ui)
    : WebUIController(web_ui) {
  web_ui->SetBindings(0);
  content::URLDataSource::Add(browser_context, new BundledDataSource());
}

}  // namespace brightray
