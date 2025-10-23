// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_ELECTRON_URL_LOADER_FACTORY_H_
#define ELECTRON_SHELL_BROWSER_NET_ELECTRON_URL_LOADER_FACTORY_H_

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/url_request/url_request_job_factory.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/self_deleting_url_loader_factory.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "v8/include/v8-array-buffer.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class Dictionary;
}  // namespace gin_helper

namespace mojo {
template <typename T>
class PendingReceiver;
}  // namespace mojo

namespace electron {

// Old Protocol API can only serve one type of response for one scheme.
enum class ProtocolType {
  kBuffer,
  kString,
  kFile,
  kHttp,
  kStream,
  kFree,  // special type for returning arbitrary type of response.
};

using StartLoadingCallback = base::OnceCallback<void(gin::Arguments*)>;
using ProtocolHandler =
    base::RepeatingCallback<void(const network::ResourceRequest&,
                                 StartLoadingCallback)>;

// scheme => (type, handler).
using HandlersMap = std::
    map<std::string, std::pair<ProtocolType, ProtocolHandler>, std::less<>>;

// Implementation of URLLoaderFactory.
class ElectronURLLoaderFactory : public network::SelfDeletingURLLoaderFactory {
 public:
  // This class binds a URLLoader receiver in the case of a redirect, waiting
  // for |FollowRedirect| to be called at which point the new request will be
  // started, and the receiver will be unbound letting a new URLLoader bind it
  class RedirectedRequest : public network::mojom::URLLoader {
   public:
    RedirectedRequest(
        const net::RedirectInfo& redirect_info,
        mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
        int32_t request_id,
        uint32_t options,
        const network::ResourceRequest& request,
        mojo::PendingRemote<network::mojom::URLLoaderClient> client,
        const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
        mojo::PendingRemote<network::mojom::URLLoaderFactory>
            target_factory_remote);
    ~RedirectedRequest() override;

    // disable copy
    RedirectedRequest(const RedirectedRequest&) = delete;
    RedirectedRequest& operator=(const RedirectedRequest&) = delete;

    // network::mojom::URLLoader:
    void FollowRedirect(
        const std::vector<std::string>& removed_headers,
        const net::HttpRequestHeaders& modified_headers,
        const net::HttpRequestHeaders& modified_cors_exempt_headers,
        const std::optional<GURL>& new_url) override;
    void SetPriority(net::RequestPriority priority,
                     int32_t intra_priority_value) override {}

    void OnTargetFactoryError();
    void DeleteThis();

   private:
    net::RedirectInfo redirect_info_;

    mojo::Receiver<network::mojom::URLLoader> loader_receiver_{this};
    int32_t request_id_;
    uint32_t options_;
    network::ResourceRequest request_;
    mojo::PendingRemote<network::mojom::URLLoaderClient> client_;
    net::MutableNetworkTrafficAnnotationTag traffic_annotation_;

    mojo::Remote<network::mojom::URLLoaderFactory> target_factory_remote_;
  };

  static mojo::PendingRemote<network::mojom::URLLoaderFactory> Create(
      ProtocolType type,
      const ProtocolHandler& handler);

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;

  static void StartLoading(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
      ProtocolType type,
      gin::Arguments* args);

  // disable copy
  ElectronURLLoaderFactory(const ElectronURLLoaderFactory&) = delete;
  ElectronURLLoaderFactory& operator=(const ElectronURLLoaderFactory&) = delete;

 private:
  ElectronURLLoaderFactory(
      ProtocolType type,
      const ProtocolHandler& handler,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver);
  ~ElectronURLLoaderFactory() override;

  static void OnComplete(
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      int32_t request_id,
      const network::URLLoaderCompletionStatus& status);

  static void StartLoadingBuffer(
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      network::mojom::URLResponseHeadPtr head,
      v8::Local<v8::ArrayBufferView> buffer);
  static void StartLoadingFile(
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      network::mojom::URLResponseHeadPtr head,
      const network::ResourceRequest& original_request,
      const base::FilePath& path,
      const gin_helper::Dictionary& opts);
  static void StartLoadingHttp(
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      const network::ResourceRequest& original_request,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      const gin_helper::Dictionary& dict);
  static void StartLoadingStream(
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      network::mojom::URLResponseHeadPtr head,
      const gin_helper::Dictionary& dict);

  // Helper to send string as response.
  static void SendContents(
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      network::mojom::URLResponseHeadPtr head,
      std::string data);

  ProtocolType type_;
  ProtocolHandler handler_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_ELECTRON_URL_LOADER_FACTORY_H_
