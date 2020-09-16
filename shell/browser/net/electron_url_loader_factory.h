// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NET_ELECTRON_URL_LOADER_FACTORY_H_
#define SHELL_BROWSER_NET_ELECTRON_URL_LOADER_FACTORY_H_

#include <map>
#include <string>
#include <utility>

#include "content/public/browser/non_network_url_loader_factory_base.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/url_request/url_request_job_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "shell/common/gin_helper/dictionary.h"

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
    base::Callback<void(const network::ResourceRequest&, StartLoadingCallback)>;

// scheme => (type, handler).
using HandlersMap =
    std::map<std::string, std::pair<ProtocolType, ProtocolHandler>>;

// Implementation of URLLoaderFactory.
class ElectronURLLoaderFactory
    : public content::NonNetworkURLLoaderFactoryBase {
 public:
  static mojo::PendingRemote<network::mojom::URLLoaderFactory> Create(
      ProtocolType type,
      const ProtocolHandler& handler);

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;

  static void StartLoading(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> proxy_factory,
      ProtocolType type,
      gin::Arguments* args);

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
      const gin_helper::Dictionary& dict);
  static void StartLoadingString(
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      network::mojom::URLResponseHeadPtr head,
      const gin_helper::Dictionary& dict,
      v8::Isolate* isolate,
      v8::Local<v8::Value> response);
  static void StartLoadingFile(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      network::ResourceRequest request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      network::mojom::URLResponseHeadPtr head,
      const gin_helper::Dictionary& dict,
      v8::Isolate* isolate,
      v8::Local<v8::Value> response);
  static void StartLoadingHttp(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      const network::ResourceRequest& original_request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      const gin_helper::Dictionary& dict);
  static void StartLoadingStream(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      network::mojom::URLResponseHeadPtr head,
      const gin_helper::Dictionary& dict);

  // Helper to send string as response.
  static void SendContents(
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      network::mojom::URLResponseHeadPtr head,
      std::string data);

  ProtocolType type_;
  ProtocolHandler handler_;

  DISALLOW_COPY_AND_ASSIGN(ElectronURLLoaderFactory);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NET_ELECTRON_URL_LOADER_FACTORY_H_
