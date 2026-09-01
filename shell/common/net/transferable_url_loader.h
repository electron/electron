// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_NET_TRANSFERABLE_URL_LOADER_H_
#define ELECTRON_SHELL_COMMON_NET_TRANSFERABLE_URL_LOADER_H_

#include <memory>
#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_span.h"
#include "base/sequence_checker.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "url/gurl.h"

namespace electron {

struct PendingURLLoaderResponse {
  mojo::ScopedDataPipeConsumerHandle body;
  std::optional<mojo_base::BigBuffer> cached_metadata;
  std::vector<int32_t> transfer_size_updates;
  std::optional<network::URLLoaderCompletionStatus> completion_status;
  network::mojom::URLLoaderClientEndpointsPtr endpoints;
};

// Interposes between SimpleURLLoader and its selected target factory so the
// original response body pipe can either be read by JavaScript or transferred
// untouched to another client.
class TransferableURLLoader final : public network::SharedURLLoaderFactory,
                                    public network::mojom::URLLoader,
                                    public network::mojom::URLLoaderClient {
 public:
  class Delegate {
   public:
    virtual void OnTransferableResponseStarted(
        const GURL& final_url,
        const network::mojom::URLResponseHead& response_head) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  TransferableURLLoader(
      Delegate* delegate,
      scoped_refptr<network::SharedURLLoaderFactory> target_url_loader_factory,
      const GURL& initial_url);

  void Cancel();
  bool CanTransfer() const;
  std::optional<PendingURLLoaderResponse> TakeResponse();
  void SetTransferredCancelCallback(base::OnceClosure callback);
  void Read(base::span<uint8_t> buffer, base::OnceCallback<void(int)> callback);

  // network::SharedURLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;
  void Clone(mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver)
      override;
  std::unique_ptr<network::PendingSharedURLLoaderFactory> Clone() override;

  // network::mojom::URLLoader:
  void FollowRedirect(
      network::HttpRequestHeadersUpdateParams headers_update_params,
      const std::optional<GURL>& new_url) override;
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override;

  // network::mojom::URLLoaderClient:
  void OnReceiveEarlyHints(network::mojom::EarlyHintsPtr early_hints) override;
  void OnReceiveResponse(
      network::mojom::URLResponseHeadPtr head,
      mojo::ScopedDataPipeConsumerHandle body,
      std::optional<mojo_base::BigBuffer> cached_metadata) override;
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         network::mojom::URLResponseHeadPtr head) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnComplete(const network::URLLoaderCompletionStatus& status) override;

 private:
  ~TransferableURLLoader() override;

  enum class Disposition {
    kUnclaimed,
    kReading,
    kTransferred,
  };

  std::optional<int> ReadInternal(base::span<uint8_t> buffer);
  void OnBodyReadable(MojoResult result);
  void OnTargetURLLoaderClientDisconnected();
  void CompletePendingRead();

  raw_ptr<Delegate> delegate_;
  scoped_refptr<network::SharedURLLoaderFactory> target_url_loader_factory_;
  GURL final_url_;
  Disposition disposition_ = Disposition::kUnclaimed;
  bool response_received_ = false;
  bool pipe_closed_ = false;
  mojo::Remote<network::mojom::URLLoader> target_url_loader_;
  mojo::Receiver<network::mojom::URLLoaderClient>
      target_url_loader_client_receiver_{this};
  mojo::Receiver<network::mojom::URLLoader> simple_url_loader_receiver_{this};
  mojo::Remote<network::mojom::URLLoaderClient> simple_url_loader_client_;
  mojo::ScopedDataPipeConsumerHandle body_;
  std::optional<mojo_base::BigBuffer> cached_metadata_;
  std::vector<int32_t> transfer_size_updates_;
  mojo::SimpleWatcher body_watcher_;
  std::optional<network::URLLoaderCompletionStatus> completion_status_;
  base::raw_span<uint8_t> pending_read_buffer_;
  base::OnceCallback<void(int)> pending_read_callback_;
  base::OnceClosure transferred_cancel_callback_;
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_NET_TRANSFERABLE_URL_LOADER_H_
