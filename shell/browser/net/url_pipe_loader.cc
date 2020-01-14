// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/url_pipe_loader.h"

#include <utility>

#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/system/string_data_source.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace electron {

URLPipeLoader::URLPipeLoader(
    scoped_refptr<network::SharedURLLoaderFactory> factory,
    std::unique_ptr<network::ResourceRequest> request,
    network::mojom::URLLoaderRequest loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::NetworkTrafficAnnotationTag& annotation,
    base::DictionaryValue upload_data)
    : binding_(this, std::move(loader)),
      client_(std::move(client)),
      weak_factory_(this) {
  binding_.set_connection_error_handler(base::BindOnce(
      &URLPipeLoader::NotifyComplete, base::Unretained(this), net::ERR_FAILED));

  // PostTask since it might destruct.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&URLPipeLoader::Start, weak_factory_.GetWeakPtr(), factory,
                     std::move(request), annotation, std::move(upload_data)));
}

URLPipeLoader::~URLPipeLoader() = default;

void URLPipeLoader::Start(
    scoped_refptr<network::SharedURLLoaderFactory> factory,
    std::unique_ptr<network::ResourceRequest> request,
    const net::NetworkTrafficAnnotationTag& annotation,
    base::DictionaryValue upload_data) {
  loader_ = network::SimpleURLLoader::Create(std::move(request), annotation);
  loader_->SetOnResponseStartedCallback(base::Bind(
      &URLPipeLoader::OnResponseStarted, weak_factory_.GetWeakPtr()));

  // TODO(zcbenz): The old protocol API only supports string as upload data,
  // we should seek to support more types in future.
  std::string content_type, data;
  if (upload_data.GetString("contentType", &content_type) &&
      upload_data.GetString("data", &data))
    loader_->AttachStringForUpload(data, content_type);

  loader_->DownloadAsStream(factory.get(), this);
}

void URLPipeLoader::NotifyComplete(int result) {
  client_->OnComplete(network::URLLoaderCompletionStatus(result));
  delete this;
}

void URLPipeLoader::OnResponseStarted(
    const GURL& final_url,
    const network::mojom::URLResponseHead& response_head) {
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  MojoResult rv = mojo::CreateDataPipe(nullptr, &producer, &consumer);
  if (rv != MOJO_RESULT_OK) {
    NotifyComplete(net::ERR_INSUFFICIENT_RESOURCES);
    return;
  }

  producer_ = std::make_unique<mojo::DataPipeProducer>(std::move(producer));

  client_->OnReceiveResponse(response_head.Clone());
  client_->OnStartLoadingResponseBody(std::move(consumer));
}

void URLPipeLoader::OnWrite(base::OnceClosure resume, MojoResult result) {
  if (result == MOJO_RESULT_OK)
    std::move(resume).Run();
  else
    NotifyComplete(net::ERR_FAILED);
}

void URLPipeLoader::OnDataReceived(base::StringPiece string_piece,
                                   base::OnceClosure resume) {
  producer_->Write(
      std::make_unique<mojo::StringDataSource>(
          string_piece, mojo::StringDataSource::AsyncWritingMode::
                            STRING_STAYS_VALID_UNTIL_COMPLETION),
      base::BindOnce(&URLPipeLoader::OnWrite, weak_factory_.GetWeakPtr(),
                     std::move(resume)));
}

void URLPipeLoader::OnRetry(base::OnceClosure start_retry) {
  NOTREACHED();
}

void URLPipeLoader::OnComplete(bool success) {
  NotifyComplete(loader_->NetError());
}

}  // namespace electron
