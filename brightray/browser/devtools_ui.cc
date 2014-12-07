// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/devtools_ui.h"

#include <string>

#include "browser/browser_context.h"

#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/resource/resource_bundle.h"

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
  } else if (EndsWith(filename, ".manifest", false)) {
    return "text/cache-manifest";
  }
  NOTREACHED();
  return "text/plain";
}

class BundledDataSource : public content::URLDataSource {
 public:
  explicit BundledDataSource() {
  }

  // content::URLDataSource implementation.
  virtual std::string GetSource() const override {
    return kChromeUIDevToolsBundledHost;
  }

  virtual void StartDataRequest(const std::string& path,
                                int render_process_id,
                                int render_view_id,
                                const GotDataCallback& callback) override {
    std::string filename = PathWithoutParams(path);

    int resource_id =
        content::DevToolsHttpHandler::GetFrontendResourceId(filename);

    DLOG_IF(WARNING, resource_id == -1)
        << "Unable to find dev tool resource: " << filename
        << ". If you compiled with debug_devtools=1, try running with "
           "--debug-devtools.";
    const ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    scoped_refptr<base::RefCountedStaticMemory> bytes(rb.LoadDataResourceBytes(
        resource_id));
    callback.Run(bytes.get());
  }

  virtual std::string GetMimeType(const std::string& path) const override {
    return GetMimeTypeForPath(path);
  }

  virtual bool ShouldAddContentSecurityPolicy() const override {
    return false;
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
