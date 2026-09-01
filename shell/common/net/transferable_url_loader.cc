// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/net/transferable_url_loader.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/numerics/safe_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace electron {

TransferableURLLoader::TransferableURLLoader(
    Delegate* delegate,
    scoped_refptr<network::SharedURLLoaderFactory> target_url_loader_factory,
    const GURL& initial_url)
    : delegate_(delegate),
      target_url_loader_factory_(std::move(target_url_loader_factory)),
      final_url_(initial_url),
      body_watcher_(FROM_HERE,
                    mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                    base::SequencedTaskRunner::GetCurrentDefault()) {}

TransferableURLLoader::~TransferableURLLoader() = default;

void TransferableURLLoader::Cancel() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  delegate_ = nullptr;
  completion_status_ = network::URLLoaderCompletionStatus(net::ERR_ABORTED);
  pipe_closed_ = true;
  if (transferred_cancel_callback_)
    std::move(transferred_cancel_callback_).Run();
  body_watcher_.Cancel();
  body_.reset();
  target_url_loader_client_receiver_.reset();
  target_url_loader_.reset();
  simple_url_loader_receiver_.reset();
  simple_url_loader_client_.reset();
  pending_read_buffer_ = {};
  if (pending_read_callback_)
    std::move(pending_read_callback_).Run(net::ERR_ABORTED);
}

bool TransferableURLLoader::CanTransfer() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return response_received_ && disposition_ == Disposition::kUnclaimed;
}

std::optional<PendingURLLoaderResponse> TransferableURLLoader::TakeResponse() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!CanTransfer())
    return std::nullopt;

  disposition_ = Disposition::kTransferred;
  body_watcher_.Cancel();
  auto target_url_loader_client_receiver =
      target_url_loader_client_receiver_.Unbind();
  simple_url_loader_receiver_.reset();
  simple_url_loader_client_.reset();
  auto endpoints = network::mojom::URLLoaderClientEndpoints::New(
      target_url_loader_.Unbind(),
      std::move(target_url_loader_client_receiver));
  return PendingURLLoaderResponse{std::move(body_), std::move(cached_metadata_),
                                  std::move(transfer_size_updates_),
                                  std::move(completion_status_),
                                  std::move(endpoints)};
}

void TransferableURLLoader::SetTransferredCancelCallback(
    base::OnceClosure callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK_EQ(disposition_, Disposition::kTransferred);
  transferred_cancel_callback_ = std::move(callback);
}

void TransferableURLLoader::Read(base::span<uint8_t> buffer,
                                 base::OnceCallback<void(int)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (disposition_ == Disposition::kTransferred) {
    std::move(callback).Run(net::ERR_UNEXPECTED);
    return;
  }
  if (!response_received_) {
    std::move(callback).Run(net::ERR_UNEXPECTED);
    return;
  }
  if (pending_read_callback_) {
    std::move(callback).Run(net::ERR_IO_PENDING);
    return;
  }
  if (disposition_ == Disposition::kUnclaimed)
    disposition_ = Disposition::kReading;

  if (auto result = ReadInternal(buffer)) {
    std::move(callback).Run(*result);
  } else {
    pending_read_buffer_ = buffer;
    pending_read_callback_ = std::move(callback);
  }
}

void TransferableURLLoader::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!simple_url_loader_receiver_.is_bound());
  simple_url_loader_receiver_.Bind(std::move(loader));
  simple_url_loader_client_.Bind(std::move(client));
  target_url_loader_factory_->CreateLoaderAndStart(
      target_url_loader_.BindNewPipeAndPassReceiver(), request_id, options,
      request, target_url_loader_client_receiver_.BindNewPipeAndPassRemote(),
      traffic_annotation);
  target_url_loader_client_receiver_.set_disconnect_handler(base::BindOnce(
      &TransferableURLLoader::OnTargetURLLoaderClientDisconnected,
      base::Unretained(this)));
}

void TransferableURLLoader::Clone(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  target_url_loader_factory_->Clone(std::move(receiver));
}

std::unique_ptr<network::PendingSharedURLLoaderFactory>
TransferableURLLoader::Clone() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return target_url_loader_factory_->Clone();
}

void TransferableURLLoader::FollowRedirect(
    network::HttpRequestHeadersUpdateParams headers_update_params,
    const std::optional<GURL>& new_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  target_url_loader_->FollowRedirect(std::move(headers_update_params), new_url);
}

void TransferableURLLoader::SetPriority(net::RequestPriority priority,
                                        int32_t intra_priority_value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  target_url_loader_->SetPriority(priority, intra_priority_value);
}

void TransferableURLLoader::OnReceiveEarlyHints(
    network::mojom::EarlyHintsPtr early_hints) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(simple_url_loader_client_.is_bound());
  simple_url_loader_client_->OnReceiveEarlyHints(std::move(early_hints));
}

void TransferableURLLoader::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr head,
    mojo::ScopedDataPipeConsumerHandle body,
    std::optional<mojo_base::BigBuffer> cached_metadata) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!response_received_);
  response_received_ = true;
  body_ = std::move(body);
  cached_metadata_ = std::move(cached_metadata);
  if (delegate_) {
    scoped_refptr<TransferableURLLoader> protect(this);
    Delegate* delegate = delegate_;
    delegate_ = nullptr;
    delegate->OnTransferableResponseStarted(final_url_, *head);
  }
}

void TransferableURLLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr head) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(simple_url_loader_client_.is_bound());
  final_url_ = redirect_info.new_url;
  simple_url_loader_client_->OnReceiveRedirect(redirect_info, std::move(head));
}

void TransferableURLLoader::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(simple_url_loader_client_.is_bound());
  simple_url_loader_client_->OnUploadProgress(current_position, total_size,
                                              std::move(callback));
}

void TransferableURLLoader::OnTransferSizeUpdated(int32_t transfer_size_diff) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!response_received_) {
    CHECK(simple_url_loader_client_.is_bound());
    simple_url_loader_client_->OnTransferSizeUpdated(transfer_size_diff);
  } else if (disposition_ == Disposition::kUnclaimed) {
    // Preserve updates only while the response may still be transferred.
    // net.fetch does not expose transfer progress for JS-consumed bodies.
    transfer_size_updates_.push_back(transfer_size_diff);
  }
}

void TransferableURLLoader::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!response_received_) {
    CHECK(simple_url_loader_client_.is_bound());
    delegate_ = nullptr;
    simple_url_loader_client_->OnComplete(status);
    return;
  }
  completion_status_ = status;
  CompletePendingRead();
}

std::optional<int> TransferableURLLoader::ReadInternal(
    base::span<uint8_t> buffer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!body_.is_valid()) {
    pipe_closed_ = true;
    if (!completion_status_)
      return std::nullopt;
    return completion_status_->error_code == net::OK
               ? 0
               : completion_status_->error_code;
  }

  size_t num_bytes = buffer.size();
  MojoResult result =
      body_->ReadData(MOJO_READ_DATA_FLAG_NONE, buffer, num_bytes);
  if (result == MOJO_RESULT_OK)
    return base::checked_cast<int>(num_bytes);
  if (result == MOJO_RESULT_SHOULD_WAIT) {
    if (!body_watcher_.IsWatching()) {
      body_watcher_.Watch(
          body_.get(),
          MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
          base::BindRepeating(&TransferableURLLoader::OnBodyReadable,
                              base::Unretained(this)));
    }
    body_watcher_.ArmOrNotify();
    return std::nullopt;
  }

  pipe_closed_ = true;
  body_watcher_.Cancel();
  body_.reset();
  if (!completion_status_)
    return std::nullopt;
  return completion_status_->error_code == net::OK
             ? 0
             : completion_status_->error_code;
}

void TransferableURLLoader::OnBodyReadable(MojoResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!pending_read_callback_)
    return;
  if (result != MOJO_RESULT_OK && result != MOJO_RESULT_FAILED_PRECONDITION) {
    auto callback = std::move(pending_read_callback_);
    pending_read_buffer_ = {};
    std::move(callback).Run(net::ERR_FAILED);
    return;
  }

  auto read_result = ReadInternal(pending_read_buffer_);
  if (!read_result)
    return;
  auto callback = std::move(pending_read_callback_);
  pending_read_buffer_ = {};
  std::move(callback).Run(*read_result);
}

void TransferableURLLoader::OnTargetURLLoaderClientDisconnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (completion_status_)
    return;
  completion_status_ = network::URLLoaderCompletionStatus(net::ERR_FAILED);
  pipe_closed_ = true;
  body_.reset();
  if (!response_received_) {
    delegate_ = nullptr;
    if (simple_url_loader_client_.is_bound())
      simple_url_loader_client_->OnComplete(*completion_status_);
    target_url_loader_client_receiver_.reset();
    target_url_loader_.reset();
    simple_url_loader_receiver_.reset();
    simple_url_loader_client_.reset();
    return;
  }
  CompletePendingRead();
}

void TransferableURLLoader::CompletePendingRead() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!pipe_closed_ || !completion_status_ || !pending_read_callback_)
    return;
  auto callback = std::move(pending_read_callback_);
  pending_read_buffer_ = {};
  std::move(callback).Run(completion_status_->error_code == net::OK
                              ? 0
                              : completion_status_->error_code);
}

}  // namespace electron
