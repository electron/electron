// Copyright (c) 2024 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/devtools_ui_bundle_data_source.h"

#include <string>
#include <utility>

#include "base/memory/ref_counted_memory.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/color/chrome_color_provider_utils.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "net/base/url_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_utils.h"

namespace electron {
namespace {
std::string PathWithoutParams(const std::string& path) {
  return GURL(base::StrCat({content::kChromeDevToolsScheme,
                            url::kStandardSchemeSeparator,
                            chrome::kChromeUIDevToolsHost}))
      .Resolve(path)
      .GetPath()
      .substr(1);
}

scoped_refptr<base::RefCountedMemory> CreateNotFoundResponse() {
  const char kHttpNotFound[] = "HTTP/1.1 404 Not Found\n\n";
  return base::MakeRefCounted<base::RefCountedStaticMemory>(
      base::byte_span_from_cstring(kHttpNotFound));
}

std::string GetMimeTypeForUrl(const GURL& url) {
  std::string filename = url.ExtractFileName();
  if (base::EndsWith(filename, ".html", base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/html";
  } else if (base::EndsWith(filename, ".css",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/css";
  } else if (base::EndsWith(filename, ".js",
                            base::CompareCase::INSENSITIVE_ASCII) ||
             base::EndsWith(filename, ".mjs",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/javascript";
  } else if (base::EndsWith(filename, ".png",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/png";
  } else if (base::EndsWith(filename, ".map",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/json";
  } else if (base::EndsWith(filename, ".ts",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/x-typescript";
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
}  // namespace

#pragma mark - BundledDataSource

std::string BundledDataSource::GetSource() {
  // content::URLDataSource implementation.
  return chrome::kChromeUIDevToolsHost;
}

void BundledDataSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  const std::string path = content::URLDataSource::URLToRequestPath(url);
  // Serve request from local bundle.
  std::string bundled_path_prefix(chrome::kChromeUIDevToolsBundledPath);
  bundled_path_prefix += "/";
  if (base::StartsWith(path, bundled_path_prefix,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    StartBundledDataRequest(path.substr(bundled_path_prefix.length()),
                            std::move(callback));
    return;
  }

  // We do not handle remote and custom requests.
  std::move(callback).Run(CreateNotFoundResponse());
}

std::string BundledDataSource::GetMimeType(const GURL& url) {
  return GetMimeTypeForUrl(url);
}

bool BundledDataSource::ShouldAddContentSecurityPolicy() {
  return false;
}

bool BundledDataSource::ShouldDenyXFrameOptions() {
  return false;
}

bool BundledDataSource::ShouldServeMimeTypeAsContentTypeHeader() {
  return true;
}

void BundledDataSource::StartBundledDataRequest(const std::string& path,
                                                GotDataCallback callback) {
  std::string filename = PathWithoutParams(path);
  scoped_refptr<base::RefCountedMemory> bytes =
      content::DevToolsFrontendHost::GetFrontendResourceBytes(filename);

  DLOG_IF(WARNING, !bytes)
      << "Unable to find dev tool resource: " << filename
      << ". If you compiled with debug_devtools=1, try running with "
         "--debug-devtools.";
  std::move(callback).Run(bytes);
}

}  // namespace electron
