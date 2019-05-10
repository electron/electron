// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_url_loader_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/node_stream_loader.h"
#include "atom/common/atom_constants.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/guid.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/file_url_loader.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/base/filename_util.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

#include "atom/common/node_includes.h"

using content::BrowserThread;

namespace mate {

template <>
struct Converter<atom::ProtocolType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     atom::ProtocolType* out) {
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "buffer")
      *out = atom::ProtocolType::kBuffer;
    else if (type == "string")
      *out = atom::ProtocolType::kString;
    else if (type == "file")
      *out = atom::ProtocolType::kFile;
    else if (type == "http")
      *out = atom::ProtocolType::kHttp;
    else if (type == "stream")
      *out = atom::ProtocolType::kStream;
    else  // note "free" is internal type, not allowed to be passed from user
      return false;
    return true;
  }
};

}  // namespace mate

namespace atom {

namespace {

// Determine whether a protocol type can accept non-object response.
bool ResponseMustBeObject(ProtocolType type) {
  switch (type) {
    case ProtocolType::kString:
    case ProtocolType::kFile:
    case ProtocolType::kFree:
      return false;
    default:
      return true;
  }
}

// Helper to convert value to Dictionary.
mate::Dictionary ToDict(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  if (value->IsObject())
    return mate::Dictionary(
        isolate,
        value->ToObject(isolate->GetCurrentContext()).ToLocalChecked());
  else
    return mate::Dictionary();
}

// Parse headers from response object.
network::ResourceResponseHead ToResponseHead(const mate::Dictionary& dict) {
  network::ResourceResponseHead head;
  head.mime_type = "text/html";
  head.charset = "utf-8";
  if (dict.IsEmpty()) {
    head.headers = new net::HttpResponseHeaders("HTTP/1.1 200 OK");
    return head;
  }

  int status_code = 200;
  dict.Get("statusCode", &status_code);
  head.headers = new net::HttpResponseHeaders(base::StringPrintf(
      "HTTP/1.1 %d %s", status_code,
      net::GetHttpReasonPhrase(static_cast<net::HttpStatusCode>(status_code))));

  base::DictionaryValue headers;
  if (dict.Get("headers", &headers)) {
    for (const auto& iter : headers.DictItems()) {
      if (iter.second.is_string()) {
        // key: value
        head.headers->AddHeader(iter.first + ": " + iter.second.GetString());
      } else if (iter.second.is_list()) {
        // key: [values...]
        for (const auto& item : iter.second.GetList()) {
          if (item.is_string())
            head.headers->AddHeader(iter.first + ": " + item.GetString());
        }
      } else {
        continue;
      }
      // Some apps are passing content-type via headers, which is not accepted
      // in NetworkService.
      if (iter.first == "content-type" && iter.second.is_string())
        head.mime_type = iter.second.GetString();
    }
  }
  dict.Get("mimeType", &head.mime_type);
  dict.Get("charset", &head.charset);
  return head;
}

// Helper to write string to pipe.
struct WriteData {
  network::mojom::URLLoaderClientPtr client;
  std::string data;
  std::unique_ptr<mojo::StringDataPipeProducer> producer;
};

void OnWrite(std::unique_ptr<WriteData> write_data, MojoResult result) {
  if (result != MOJO_RESULT_OK) {
    network::URLLoaderCompletionStatus status(net::ERR_FAILED);
    return;
  }

  network::URLLoaderCompletionStatus status(net::OK);
  status.encoded_data_length = write_data->data.size();
  status.encoded_body_length = write_data->data.size();
  status.decoded_body_length = write_data->data.size();
  write_data->client->OnComplete(status);
}

}  // namespace

AtomURLLoaderFactory::AtomURLLoaderFactory(ProtocolType type,
                                           const ProtocolHandler& handler)
    : type_(type), handler_(handler) {}

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
  handler_.Run(
      request,
      base::BindOnce(&AtomURLLoaderFactory::StartLoading, std::move(loader),
                     routing_id, request_id, options, request,
                     std::move(client), traffic_annotation, type_));
}

void AtomURLLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

// static
void AtomURLLoaderFactory::StartLoading(
    network::mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    ProtocolType type,
    v8::Local<v8::Value> response,
    mate::Arguments* args) {
  // Parse {error} object.
  mate::Dictionary dict = ToDict(args->isolate(), response);
  if (!dict.IsEmpty()) {
    int error_code;
    if (dict.Get("error", &error_code)) {
      client->OnComplete(network::URLLoaderCompletionStatus(error_code));
      return;
    }
  }

  // Some protocol accepts non-object responses.
  if (dict.IsEmpty() && ResponseMustBeObject(type)) {
    client->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_NOT_IMPLEMENTED));
    return;
  }

  switch (type) {
    case ProtocolType::kBuffer:
      StartLoadingBuffer(std::move(client), dict);
      break;
    case ProtocolType::kString:
      StartLoadingString(std::move(client), dict, args->isolate(), response);
      break;
    case ProtocolType::kFile:
      StartLoadingFile(std::move(loader), request, std::move(client), dict,
                       args->isolate(), response);
      break;
    case ProtocolType::kHttp:
      StartLoadingHttp(std::move(loader), routing_id, request_id, options,
                       request, std::move(client), traffic_annotation, dict);
      break;
    case ProtocolType::kStream:
      StartLoadingStream(std::move(loader), std::move(client), dict);
      break;
    case ProtocolType::kFree:
      ProtocolType type;
      v8::Local<v8::Value> extra_arg;
      if (!mate::ConvertFromV8(args->isolate(), response, &type) ||
          !args->GetNext(&extra_arg)) {
        client->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
        args->ThrowError("Invalid args, must pass (type, options)");
        return;
      }
      StartLoading(std::move(loader), routing_id, request_id, options, request,
                   std::move(client), traffic_annotation, type, extra_arg,
                   args);
      break;
  }
}

// static
void AtomURLLoaderFactory::StartLoadingBuffer(
    network::mojom::URLLoaderClientPtr client,
    const mate::Dictionary& dict) {
  v8::Local<v8::Value> buffer = dict.GetHandle();
  dict.Get("data", &buffer);
  if (!node::Buffer::HasInstance(buffer)) {
    client->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  SendContents(
      std::move(client), ToResponseHead(dict),
      std::string(node::Buffer::Data(buffer), node::Buffer::Length(buffer)));
}

// static
void AtomURLLoaderFactory::StartLoadingString(
    network::mojom::URLLoaderClientPtr client,
    const mate::Dictionary& dict,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response) {
  std::string contents;
  if (response->IsString())
    contents = gin::V8ToString(isolate, response);
  else if (!dict.IsEmpty())
    dict.Get("data", &contents);

  SendContents(std::move(client), ToResponseHead(dict), std::move(contents));
}

// static
void AtomURLLoaderFactory::StartLoadingFile(
    network::mojom::URLLoaderRequest loader,
    network::ResourceRequest request,
    network::mojom::URLLoaderClientPtr client,
    const mate::Dictionary& dict,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response) {
  base::FilePath path;
  if (mate::ConvertFromV8(isolate, response, &path)) {
    request.url = net::FilePathToFileURL(path);
  } else if (!dict.IsEmpty()) {
    dict.Get("referrer", &request.referrer);
    dict.Get("method", &request.method);
    if (dict.Get("path", &path))
      request.url = net::FilePathToFileURL(path);
  } else {
    client->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  network::ResourceResponseHead head = ToResponseHead(dict);
  head.headers->AddHeader(kCORSHeader);
  content::CreateFileURLLoader(request, std::move(loader), std::move(client),
                               nullptr, false, head.headers);
}

// static
void AtomURLLoaderFactory::StartLoadingHttp(
    network::mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& original_request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    const mate::Dictionary& dict) {
  network::ResourceRequest request;
  request.headers = original_request.headers;
  request.cors_exempt_headers = original_request.cors_exempt_headers;

  dict.Get("url", &request.url);
  dict.Get("referrer", &request.referrer);
  if (!dict.Get("method", &request.method))
    request.method = original_request.method;

  scoped_refptr<AtomBrowserContext> browser_context =
      AtomBrowserContext::From("", false);
  v8::Local<v8::Value> value;
  if (dict.Get("session", &value)) {
    if (value->IsNull()) {
      browser_context = AtomBrowserContext::From(base::GenerateGUID(), true);
    } else {
      mate::Handle<api::Session> session;
      if (mate::ConvertFromV8(dict.isolate(), value, &session) &&
          !session.IsEmpty()) {
        browser_context = session->browser_context();
      }
    }
  }

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      content::BrowserContext::GetDefaultStoragePartition(browser_context.get())
          ->GetURLLoaderFactoryForBrowserProcess();
  url_loader_factory->CreateLoaderAndStart(
      std::move(loader), routing_id, request_id, options, std::move(request),
      std::move(client), traffic_annotation);
}

// static
void AtomURLLoaderFactory::StartLoadingStream(
    network::mojom::URLLoaderRequest loader,
    network::mojom::URLLoaderClientPtr client,
    const mate::Dictionary& dict) {
  network::ResourceResponseHead head = ToResponseHead(dict);
  v8::Local<v8::Value> stream;
  if (!dict.Get("data", &stream)) {
    // Assume the opts is already a stream.
    stream = dict.GetHandle();
  } else if (stream->IsNullOrUndefined()) {
    // "data" was explicitly passed as null or undefined, assume the user wants
    // to send an empty body.
    //
    // Note that We must submit a empty body otherwise NetworkService would
    // crash.
    client->OnReceiveResponse(head);
    mojo::ScopedDataPipeProducerHandle producer;
    mojo::ScopedDataPipeConsumerHandle consumer;
    if (mojo::CreateDataPipe(nullptr, &producer, &consumer) != MOJO_RESULT_OK) {
      client->OnComplete(
          network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
      return;
    }
    producer.reset();  // The data pipe is empty.
    client->OnStartLoadingResponseBody(std::move(consumer));
    client->OnComplete(network::URLLoaderCompletionStatus(net::OK));
    return;
  } else if (!stream->IsObject()) {
    client->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  mate::Dictionary data = ToDict(dict.isolate(), stream);
  v8::Local<v8::Value> method;
  if (!data.Get("on", &method) || !method->IsFunction() ||
      !data.Get("removeListener", &method) || !method->IsFunction()) {
    client->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  new NodeStreamLoader(std::move(head), std::move(loader), std::move(client),
                       data.isolate(), data.GetHandle());
}

// static
void AtomURLLoaderFactory::SendContents(
    network::mojom::URLLoaderClientPtr client,
    network::ResourceResponseHead head,
    std::string data) {
  head.headers->AddHeader(kCORSHeader);
  client->OnReceiveResponse(head);

  // Code bellow follows the pattern of data_url_loader_factory.cc.
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  if (mojo::CreateDataPipe(nullptr, &producer, &consumer) != MOJO_RESULT_OK) {
    client->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
    return;
  }

  client->OnStartLoadingResponseBody(std::move(consumer));

  auto write_data = std::make_unique<WriteData>();
  write_data->client = std::move(client);
  write_data->data = std::move(data);
  write_data->producer =
      std::make_unique<mojo::StringDataPipeProducer>(std::move(producer));

  base::StringPiece string_piece(write_data->data);
  write_data->producer->Write(string_piece,
                              mojo::StringDataPipeProducer::AsyncWritingMode::
                                  STRING_STAYS_VALID_UNTIL_COMPLETION,
                              base::BindOnce(OnWrite, std::move(write_data)));
}

}  // namespace atom
