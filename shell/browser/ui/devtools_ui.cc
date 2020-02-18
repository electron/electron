// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/ui/devtools_ui.h"

#include <list>
#include <memory>
#include <string>
#include <utility>

#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/devtools/devtools_ui_bindings.h"
#include "chrome/browser/devtools/url_constants.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace electron {

namespace {

const char kChromeUIDevToolsHost[] = "devtools";
const char kChromeUIDevToolsBundledPath[] = "bundled";
const char kChromeUIDevToolsRemotePath[] = "remote";

std::string PathWithoutParams(const std::string& path) {
  return GURL(std::string("devtools://devtools/") + path).path().substr(1);
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

scoped_refptr<base::RefCountedMemory> CreateNotFoundResponse() {
  const char kHttpNotFound[] = "HTTP/1.1 404 Not Found\n\n";
  return base::MakeRefCounted<base::RefCountedStaticMemory>(
      kHttpNotFound, strlen(kHttpNotFound));
}

class BundledDataSource : public content::URLDataSource {
 public:
  BundledDataSource(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
      : url_loader_factory_(std::move(url_loader_factory)) {}
  ~BundledDataSource() override = default;

  struct PendingRequest {
    PendingRequest() = default;

    PendingRequest(PendingRequest&& other) = default;

    ~PendingRequest() {
      if (callback)
        std::move(callback).Run(CreateNotFoundResponse());
    }
    PendingRequest& operator=(PendingRequest&& other) = default;

    GotDataCallback callback;
    std::unique_ptr<network::SimpleURLLoader> loader;

    DISALLOW_COPY_AND_ASSIGN(PendingRequest);
  };

  // content::URLDataSource implementation.
  std::string GetSource() override { return kChromeUIDevToolsHost; }

  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override {
    const std::string path = content::URLDataSource::URLToRequestPath(url);
    // Serve request from local bundle.
    std::string bundled_path_prefix(kChromeUIDevToolsBundledPath);
    bundled_path_prefix += "/";
    LOG(ERROR) << "Got Request: " << url.spec();
    if (base::StartsWith(path, bundled_path_prefix,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      StartBundledDataRequest(path.substr(bundled_path_prefix.length()),
                              std::move(callback));
      return;
    }

    std::string remote_path_prefix(kChromeUIDevToolsRemotePath);
    remote_path_prefix += "/";
    if (base::StartsWith(path, remote_path_prefix,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      GURL url(kRemoteFrontendBase + path.substr(remote_path_prefix.length()));

      CHECK_EQ(url.host(), kRemoteFrontendDomain);
      if (url.is_valid() && DevToolsUIBindings::IsValidRemoteFrontendURL(url)) {
        StartRemoteDataRequest(url, std::move(callback));
      } else {
        DLOG(ERROR) << "Refusing to load invalid remote front-end URL";
        std::move(callback).Run(CreateNotFoundResponse());
      }
      return;
    }

    // We do not handle remote and custom requests.
    std::move(callback).Run(nullptr);
  }

  std::string GetMimeType(const std::string& path) override {
    return GetMimeTypeForPath(path);
  }

  bool ShouldAddContentSecurityPolicy() override { return false; }

  bool ShouldDenyXFrameOptions() override { return false; }

  bool ShouldServeMimeTypeAsContentTypeHeader() override { return true; }

  void StartBundledDataRequest(const std::string& path,
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

  void StartRemoteDataRequest(
      const GURL& url,
      content::URLDataSource::GotDataCallback callback) {
    CHECK(url.is_valid());
    net::NetworkTrafficAnnotationTag traffic_annotation =
        net::DefineNetworkTrafficAnnotation("devtools_hard_coded_data_source",
                                            R"(
        semantics {
          sender: "Developer Tools Remote Data Request From Google"
          description:
            "This service fetches Chromium DevTools front-end files from the "
            "cloud for the remote debugging scenario."
          trigger:
            "When user attaches to mobile phone for debugging."
          data: "None"
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting: "This feature cannot be disabled by settings."
          chrome_policy {
            DeveloperToolsAvailability {
              policy_options {mode: MANDATORY}
              DeveloperToolsAvailability: 2
            }
          }
        })");

    StartNetworkRequest(url, traffic_annotation, std::move(callback));
  }

  void StartNetworkRequest(
      const GURL& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      GotDataCallback callback) {
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = url;

    auto request_iter = pending_requests_.emplace(pending_requests_.begin());
    request_iter->callback = std::move(callback);
    request_iter->loader = network::SimpleURLLoader::Create(std::move(request),
                                                            traffic_annotation);
    request_iter->loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        url_loader_factory_.get(),
        base::BindOnce(&BundledDataSource::OnLoadComplete,
                       base::Unretained(this), request_iter));
  }

  void OnLoadComplete(std::list<PendingRequest>::iterator request_iter,
                      std::unique_ptr<std::string> response_body) {
    std::move(request_iter->callback)
        .Run(response_body
                 ? base::RefCountedString::TakeString(response_body.get())
                 : CreateNotFoundResponse());
    pending_requests_.erase(request_iter);
  }

 private:
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::list<PendingRequest> pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(BundledDataSource);
};

}  // namespace

DevToolsUI::DevToolsUI(content::BrowserContext* browser_context,
                       content::WebUI* web_ui)
    : WebUIController(web_ui) {
  auto factory = content::BrowserContext::GetDefaultStoragePartition(
                     web_ui->GetWebContents()->GetBrowserContext())
                     ->GetURLLoaderFactoryForBrowserProcess();
  web_ui->SetBindings(0);
  content::URLDataSource::Add(
      browser_context, std::make_unique<BundledDataSource>(std::move(factory)));
}

}  // namespace electron
