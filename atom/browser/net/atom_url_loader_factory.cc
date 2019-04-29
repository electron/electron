// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_url_loader_factory.h"

#include <string>
#include <utility>

#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/file_url_loader.h"
#include "gin/dictionary.h"
#include "net/base/filename_util.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_loader.mojom.h"

#include "atom/common/node_includes.h"

using content::BrowserThread;

namespace atom {

AtomURLLoaderFactory::AtomURLLoaderFactory(ProtocolType type,
                                           const ProtocolHandler& handler)
    : type_(type), handler_(handler), weak_factory_(this) {}

AtomURLLoaderFactory::~AtomURLLoaderFactory() = default;

void AtomURLLoaderFactory::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Context::Scope context_scope(context);

  switch (type_) {
    case ProtocolType::kBuffer:
      handler_.Run(request,
                   base::BindOnce(&AtomURLLoaderFactory::SendResponseBuffer,
                                  weak_factory_.GetWeakPtr(), std::move(client),
                                  isolate));
      break;
    case ProtocolType::kString:
      handler_.Run(request,
                   base::BindOnce(&AtomURLLoaderFactory::SendResponseString,
                                  weak_factory_.GetWeakPtr(), std::move(client),
                                  isolate));
      break;
    case ProtocolType::kFile:
      handler_.Run(request,
                   base::BindOnce(&AtomURLLoaderFactory::SendResponseFile,
                                  weak_factory_.GetWeakPtr(), std::move(loader),
                                  request, std::move(client), isolate));
      break;
    default: {
      std::string contents = "Not Implemented";
      SendContents(std::move(client), "text/html", "utf-8", contents.data(),
                   contents.size());
    }
  }
}

void AtomURLLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void AtomURLLoaderFactory::SendResponseBuffer(
    network::mojom::URLLoaderClientPtr client,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response) {
  if (HandleError(&client, isolate, response))
    return;

  std::string mime_type = "text/html";
  std::string charset = "utf-8";
  v8::Local<v8::Value> buffer;
  if (node::Buffer::HasInstance(response)) {
    buffer = response;
  } else if (response->IsObject()) {
    gin::Dictionary dict(
        isolate,
        response->ToObject(isolate->GetCurrentContext()).ToLocalChecked());
    dict.Get("mimeType", &mime_type);
    dict.Get("charset", &charset);
    dict.Get("data", &buffer);
    if (!node::Buffer::HasInstance(response))
      buffer = v8::Local<v8::Value>();
  }

  if (buffer.IsEmpty()) {
    network::URLLoaderCompletionStatus status;
    status.error_code = net::ERR_NOT_IMPLEMENTED;
    client->OnComplete(status);
    return;
  }

  SendContents(std::move(client), std::move(mime_type), std::move(charset),
               node::Buffer::Data(buffer), node::Buffer::Length(buffer));
}

void AtomURLLoaderFactory::SendResponseString(
    network::mojom::URLLoaderClientPtr client,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response) {
  if (HandleError(&client, isolate, response))
    return;

  std::string mime_type = "text/html";
  std::string charset = "utf-8";
  std::string contents;
  if (response->IsString()) {
    contents = gin::V8ToString(isolate, response);
  } else if (response->IsObject()) {
    gin::Dictionary dict(
        isolate,
        response->ToObject(isolate->GetCurrentContext()).ToLocalChecked());
    dict.Get("mimeType", &mime_type);
    dict.Get("charset", &charset);
    dict.Get("data", &contents);
  }
  SendContents(std::move(client), std::move(mime_type), std::move(charset),
               contents.data(), contents.size());
}

void AtomURLLoaderFactory::SendResponseFile(
    network::mojom::URLLoaderRequest loader,
    network::ResourceRequest request,
    network::mojom::URLLoaderClientPtr client,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response) {
  if (HandleError(&client, isolate, response))
    return;

  base::FilePath path;
  if (!mate::ConvertFromV8(isolate, response, &path)) {
    network::URLLoaderCompletionStatus status;
    status.error_code = net::ERR_NOT_IMPLEMENTED;
    client->OnComplete(status);
    return;
  }

  request.url = net::FilePathToFileURL(path);
  content::CreateFileURLLoader(request, std::move(loader), std::move(client),
                               nullptr, false);
}

bool AtomURLLoaderFactory::HandleError(
    network::mojom::URLLoaderClientPtr* client,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response) {
  if (!response->IsObject())
    return false;
  v8::Local<v8::Object> obj =
      response->ToObject(isolate->GetCurrentContext()).ToLocalChecked();
  network::URLLoaderCompletionStatus status;
  if (!gin::Dictionary(isolate, obj).Get("error", &status.error_code))
    return false;
  std::move(*client)->OnComplete(status);
  return true;
}

void AtomURLLoaderFactory::SendContents(
    network::mojom::URLLoaderClientPtr client,
    std::string mime_type,
    std::string charset,
    const char* data,
    size_t ssize) {
  uint32_t size = base::saturated_cast<uint32_t>(ssize);
  mojo::DataPipe pipe(size);
  MojoResult result =
      pipe.producer_handle->WriteData(data, &size, MOJO_WRITE_DATA_FLAG_NONE);
  if (result != MOJO_RESULT_OK || size < ssize) {
    client->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  network::ResourceResponseHead head;
  head.mime_type = std::move(mime_type);
  head.charset = std::move(charset);
  client->OnReceiveResponse(head);
  client->OnStartLoadingResponseBody(std::move(pipe.consumer_handle));
  client->OnComplete(network::URLLoaderCompletionStatus(net::OK));
}

}  // namespace atom
