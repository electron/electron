// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_URL_PIPE_LOADER_H_
#define ELECTRON_SHELL_BROWSER_NET_URL_PIPE_LOADER_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/values.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace mojo {
class DataPipeProducer;
}

namespace network {
class SharedURLLoaderFactory;
}

namespace electron {

// Read data from URL and pipe it to NetworkService.
//
// Different from creating a new loader for the URL directly, protocol handlers
// using this loader can work around CORS restrictions.
//
// This class manages its own lifetime and should delete itself when the
// connection is lost or finished.
class URLPipeLoader : public network::mojom::URLLoader,
                      public network::SimpleURLLoaderStreamConsumer {
 public:
  URLPipeLoader(scoped_refptr<network::SharedURLLoaderFactory> factory,
                std::unique_ptr<network::ResourceRequest> request,
                mojo::PendingReceiver<network::mojom::URLLoader> loader,
                mojo::PendingRemote<network::mojom::URLLoaderClient> client,
                const net::NetworkTrafficAnnotationTag& annotation,
                base::Value::Dict upload_data);

  // disable copy
  URLPipeLoader(const URLPipeLoader&) = delete;
  URLPipeLoader& operator=(const URLPipeLoader&) = delete;

 private:
  ~URLPipeLoader() override;

  void Start(scoped_refptr<network::SharedURLLoaderFactory> factory,
             std::unique_ptr<network::ResourceRequest> request,
             const net::NetworkTrafficAnnotationTag& annotation,
             base::Value::Dict upload_data);
  void NotifyComplete(int result);
  void OnResponseStarted(const GURL& final_url,
                         const network::mojom::URLResponseHead& response_head);
  void OnWrite(base::OnceClosure resume, MojoResult result);

  // SimpleURLLoaderStreamConsumer:
  void OnDataReceived(std::string_view string_view,
                      base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override;

  // URLLoader:
  void FollowRedirect(
      const std::vector<std::string>& removed_headers,
      const net::HttpRequestHeaders& modified_headers,
      const net::HttpRequestHeaders& modified_cors_exempt_headers,
      const std::optional<GURL>& new_url) override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

  mojo::Receiver<network::mojom::URLLoader> url_loader_;
  mojo::Remote<network::mojom::URLLoaderClient> client_;

  std::unique_ptr<mojo::DataPipeProducer> producer_;
  std::unique_ptr<network::SimpleURLLoader> loader_;

  base::WeakPtrFactory<URLPipeLoader> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_URL_PIPE_LOADER_H_
