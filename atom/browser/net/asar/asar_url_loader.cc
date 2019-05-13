// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/asar/asar_url_loader.h"

#include "atom/common/asar/archive.h"
#include "atom/common/asar/asar_util.h"
#include "content/public/browser/file_url_loader.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/filename_util.h"
#include "net/base/mime_sniffer.h"

namespace asar {

namespace {

constexpr size_t kDefaultFileUrlPipeSize = 65536;

// Because this makes things simpler.
static_assert(kDefaultFileUrlPipeSize >= net::kMaxBytesToSniff,
              "Default file data pipe size must be at least as large as a MIME-"
              "type sniffing buffer.");

// Rewritten from the |FileURLLoader| in |file_url_loader_factory.cc|, to serve
// asar files instead of normal files.
class AsarURLLoader : public network::mojom::URLLoader {
 public:
  static void CreateAndStart(
      const network::ResourceRequest& request,
      network::mojom::URLLoaderRequest loader,
      network::mojom::URLLoaderClientPtr client,
      scoped_refptr<net::HttpResponseHeaders> extra_response_headers) {
    // Owns itself. Will live as long as its URLLoader and URLLoaderClientPtr
    // bindings are alive - essentially until either the client gives up or all
    // file data has been sent to it.
    auto* asar_url_loader = new AsarURLLoader;
    asar_url_loader->Start(request, std::move(loader), std::move(client),
                           std::move(extra_response_headers));
  }

  // network::mojom::URLLoader:
  void FollowRedirect(const std::vector<std::string>& removed_headers,
                      const net::HttpRequestHeaders& modified_headers,
                      const base::Optional<GURL>& new_url) override {}
  void ProceedWithResponse() override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

 private:
  AsarURLLoader() : binding_(this) {}
  ~AsarURLLoader() override = default;

  void Start(const network::ResourceRequest& request,
             network::mojom::URLLoaderRequest loader,
             network::mojom::URLLoaderClientPtr client,
             scoped_refptr<net::HttpResponseHeaders> extra_response_headers) {
    network::ResourceResponseHead head;
    head.request_start = base::TimeTicks::Now();
    head.response_start = base::TimeTicks::Now();
    head.headers = extra_response_headers;
    binding_.Bind(std::move(loader));
    binding_.set_connection_error_handler(base::BindOnce(
        &AsarURLLoader::OnConnectionError, base::Unretained(this)));

    base::FilePath path;
    if (!net::FileURLToFilePath(request.url, &path)) {
      OnClientComplete(net::ERR_FAILED);
      return;
    }

    // Determine whether it is an asar file.
    base::FilePath asar_path, relative_path;
    if (!GetAsarArchivePath(path, &asar_path, &relative_path)) {
      content::CreateFileURLLoader(request, std::move(loader),
                                   std::move(client), nullptr, false,
                                   extra_response_headers);
      MaybeDeleteSelf();
      return;
    }

    client_ = std::move(client);
    OnClientComplete(net::ERR_NOT_IMPLEMENTED);
  }

  void OnConnectionError() {
    binding_.Close();
    MaybeDeleteSelf();
  }

  void OnClientComplete(net::Error net_error) {
    client_->OnComplete(network::URLLoaderCompletionStatus(net_error));
    client_.reset();
    MaybeDeleteSelf();
  }

  void MaybeDeleteSelf() {
    if (!binding_.is_bound() && !client_.is_bound())
      delete this;
  }

  mojo::Binding<network::mojom::URLLoader> binding_;
  network::mojom::URLLoaderClientPtr client_;

  DISALLOW_COPY_AND_ASSIGN(AsarURLLoader);
};

}  // namespace

void CreateAsarURLLoader(
    const network::ResourceRequest& request,
    network::mojom::URLLoaderRequest loader,
    network::mojom::URLLoaderClientPtr client,
    scoped_refptr<net::HttpResponseHeaders> extra_response_headers) {
  AsarURLLoader::CreateAndStart(request, std::move(loader), std::move(client),
                                extra_response_headers);
}

}  // namespace asar
