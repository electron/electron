// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/electron_url_loader_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "base/guid.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "mojo/public/cpp/system/string_data_source.h"
#include "net/base/filename_util.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/net/asar/asar_url_loader.h"
#include "shell/browser/net/node_stream_loader.h"
#include "shell/browser/net/url_pipe_loader.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/value_converter.h"

#include "shell/common/node_includes.h"

using content::BrowserThread;

namespace gin {

template <>
struct Converter<electron::ProtocolType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::ProtocolType* out) {
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "buffer")
      *out = electron::ProtocolType::kBuffer;
    else if (type == "string")
      *out = electron::ProtocolType::kString;
    else if (type == "file")
      *out = electron::ProtocolType::kFile;
    else if (type == "http")
      *out = electron::ProtocolType::kHttp;
    else if (type == "stream")
      *out = electron::ProtocolType::kStream;
    else  // note "free" is internal type, not allowed to be passed from user
      return false;
    return true;
  }
};

}  // namespace gin

namespace electron {

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
gin::Dictionary ToDict(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  if (!value->IsFunction() && value->IsObject())
    return gin::Dictionary(
        isolate,
        value->ToObject(isolate->GetCurrentContext()).ToLocalChecked());
  else
    return gin::Dictionary(isolate);
}

// Parse headers from response object.
network::mojom::URLResponseHeadPtr ToResponseHead(
    const gin_helper::Dictionary& dict) {
  auto head = network::mojom::URLResponseHead::New();
  head->mime_type = "text/html";
  head->charset = "utf-8";
  if (dict.IsEmpty()) {
    head->headers = new net::HttpResponseHeaders("HTTP/1.1 200 OK");
    return head;
  }

  int status_code = 200;
  dict.Get("statusCode", &status_code);
  head->headers = new net::HttpResponseHeaders(base::StringPrintf(
      "HTTP/1.1 %d %s", status_code,
      net::GetHttpReasonPhrase(static_cast<net::HttpStatusCode>(status_code))));

  dict.Get("charset", &head->charset);
  bool has_mime_type = dict.Get("mimeType", &head->mime_type);
  bool has_content_type = false;

  base::DictionaryValue headers;
  if (dict.Get("headers", &headers)) {
    for (const auto& iter : headers.DictItems()) {
      if (iter.second.is_string()) {
        // key, value
        head->headers->AddHeader(iter.first, iter.second.GetString());
      } else if (iter.second.is_list()) {
        // key: [values...]
        for (const auto& item : iter.second.GetList()) {
          if (item.is_string())
            head->headers->AddHeader(iter.first, item.GetString());
        }
      } else {
        continue;
      }
      // Some apps are passing content-type via headers, which is not accepted
      // in NetworkService.
      if (base::ToLowerASCII(iter.first) == "content-type") {
        head->headers->GetMimeTypeAndCharset(&head->mime_type, &head->charset);
        has_content_type = true;
      }
    }
  }

  // Setting |head->mime_type| does not automatically set the "content-type"
  // header in NetworkService.
  if (has_mime_type && !has_content_type)
    head->headers->AddHeader("content-type", head->mime_type);
  return head;
}

// Helper to write string to pipe.
struct WriteData {
  mojo::Remote<network::mojom::URLLoaderClient> client;
  std::string data;
  std::unique_ptr<mojo::DataPipeProducer> producer;
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

ElectronURLLoaderFactory::ElectronURLLoaderFactory(
    ProtocolType type,
    const ProtocolHandler& handler)
    : type_(type), handler_(handler) {}

ElectronURLLoaderFactory::~ElectronURLLoaderFactory() = default;

void ElectronURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  handler_.Run(
      request,
      base::BindOnce(&ElectronURLLoaderFactory::StartLoading, std::move(loader),
                     routing_id, request_id, options, request,
                     std::move(client), traffic_annotation, nullptr, type_));
}

void ElectronURLLoaderFactory::Clone(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver) {
  receivers_.Add(this, std::move(receiver));
}

// static
void ElectronURLLoaderFactory::StartLoading(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    network::mojom::URLLoaderFactory* proxy_factory,
    ProtocolType type,
    gin::Arguments* args) {
  // Send network error when there is no argument passed.
  //
  // Note that we should not throw JS error in the callback no matter what is
  // passed, to keep compatibility with old code.
  v8::Local<v8::Value> response;
  if (!args->GetNext(&response)) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_NOT_IMPLEMENTED));
    return;
  }

  // Parse {error} object.
  gin_helper::Dictionary dict = ToDict(args->isolate(), response);
  if (!dict.IsEmpty()) {
    int error_code;
    if (dict.Get("error", &error_code)) {
      mojo::Remote<network::mojom::URLLoaderClient> client_remote(
          std::move(client));
      client_remote->OnComplete(network::URLLoaderCompletionStatus(error_code));
      return;
    }
  }

  network::mojom::URLResponseHeadPtr head = ToResponseHead(dict);

  // Handle redirection.
  //
  // Note that with NetworkService, sending the "Location" header no longer
  // automatically redirects the request, we have explicitly create a new loader
  // to implement redirection. This is also what Chromium does with WebRequest
  // API in WebRequestProxyingURLLoaderFactory.
  std::string location;
  if (head->headers->IsRedirect(&location)) {
    GURL new_location = GURL(location);
    net::SiteForCookies new_site_for_cookies =
        net::SiteForCookies::FromUrl(new_location);
    network::ResourceRequest new_request = request;
    new_request.url = new_location;
    new_request.site_for_cookies = new_site_for_cookies;

    net::RedirectInfo redirect_info;
    redirect_info.status_code = head->headers->response_code();
    redirect_info.new_method = request.method;
    redirect_info.new_url = new_location;
    redirect_info.new_site_for_cookies = new_site_for_cookies;
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));

    client_remote->OnReceiveRedirect(redirect_info, std::move(head));

    // Unound client, so it an be passed to sub-methods
    client = client_remote.Unbind();
    // When the redirection comes from an intercepted scheme (which has
    // |proxy_factory| passed), we askes the proxy factory to create a loader
    // for new URL, otherwise we call |StartLoadingHttp|, which creates
    // loader with default factory.
    //
    // Note that when handling requests for intercepted scheme, creating loader
    // with default factory (i.e. calling StartLoadingHttp) would bypass the
    // ProxyingURLLoaderFactory, we have to explicitly use the proxy factory to
    // create loader so it is possible to have handlers of intercepted scheme
    // getting called recursively, which is a behavior expected in protocol
    // module.
    //
    // I'm not sure whether this is an intended behavior in Chromium.
    if (proxy_factory) {
      proxy_factory->CreateLoaderAndStart(
          std::move(loader), routing_id, request_id, options, new_request,
          std::move(client), traffic_annotation);
    } else {
      StartLoadingHttp(std::move(loader), new_request, std::move(client),
                       traffic_annotation,
                       gin::Dictionary::CreateEmpty(args->isolate()));
    }
    return;
  }

  // Some protocol accepts non-object responses.
  if (dict.IsEmpty() && ResponseMustBeObject(type)) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_NOT_IMPLEMENTED));
    return;
  }

  switch (type) {
    case ProtocolType::kBuffer:
      StartLoadingBuffer(std::move(client), std::move(head), dict);
      break;
    case ProtocolType::kString:
      StartLoadingString(std::move(client), std::move(head), dict,
                         args->isolate(), response);
      break;
    case ProtocolType::kFile:
      StartLoadingFile(std::move(loader), request, std::move(client),
                       std::move(head), dict, args->isolate(), response);
      break;
    case ProtocolType::kHttp:
      StartLoadingHttp(std::move(loader), request, std::move(client),
                       traffic_annotation, dict);
      break;
    case ProtocolType::kStream:
      StartLoadingStream(std::move(loader), std::move(client), std::move(head),
                         dict);
      break;
    case ProtocolType::kFree:
      ProtocolType type;
      if (!gin::ConvertFromV8(args->isolate(), response, &type)) {
        mojo::Remote<network::mojom::URLLoaderClient> client_remote(
            std::move(client));
        client_remote->OnComplete(
            network::URLLoaderCompletionStatus(net::ERR_FAILED));
        return;
      }
      StartLoading(std::move(loader), routing_id, request_id, options, request,
                   std::move(client), traffic_annotation, proxy_factory, type,
                   args);
      break;
  }
}

// static
void ElectronURLLoaderFactory::StartLoadingBuffer(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    const gin_helper::Dictionary& dict) {
  v8::Local<v8::Value> buffer = dict.GetHandle();
  dict.Get("data", &buffer);
  if (!node::Buffer::HasInstance(buffer)) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  SendContents(
      std::move(client), std::move(head),
      std::string(node::Buffer::Data(buffer), node::Buffer::Length(buffer)));
}

// static
void ElectronURLLoaderFactory::StartLoadingString(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    const gin_helper::Dictionary& dict,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response) {
  std::string contents;
  if (response->IsString()) {
    contents = gin::V8ToString(isolate, response);
  } else if (!dict.IsEmpty()) {
    dict.Get("data", &contents);
  } else {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  SendContents(std::move(client), std::move(head), std::move(contents));
}

// static
void ElectronURLLoaderFactory::StartLoadingFile(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    network::ResourceRequest request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    const gin_helper::Dictionary& dict,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response) {
  base::FilePath path;
  if (gin::ConvertFromV8(isolate, response, &path)) {
    request.url = net::FilePathToFileURL(path);
  } else if (!dict.IsEmpty()) {
    dict.Get("referrer", &request.referrer);
    dict.Get("method", &request.method);
    if (dict.Get("path", &path))
      request.url = net::FilePathToFileURL(path);
  } else {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  // Add header to ignore CORS.
  head->headers->AddHeader("Access-Control-Allow-Origin", "*");
  asar::CreateAsarURLLoader(request, std::move(loader), std::move(client),
                            head->headers);
}

// static
void ElectronURLLoaderFactory::StartLoadingHttp(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    const network::ResourceRequest& original_request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    const gin_helper::Dictionary& dict) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->headers = original_request.headers;
  request->cors_exempt_headers = original_request.cors_exempt_headers;

  dict.Get("url", &request->url);
  dict.Get("referrer", &request->referrer);
  if (!dict.Get("method", &request->method))
    request->method = original_request.method;

  base::DictionaryValue upload_data;
  if (request->method != "GET" && request->method != "HEAD")
    dict.Get("uploadData", &upload_data);

  scoped_refptr<ElectronBrowserContext> browser_context =
      ElectronBrowserContext::From("", false);
  v8::Local<v8::Value> value;
  if (dict.Get("session", &value)) {
    if (value->IsNull()) {
      browser_context =
          ElectronBrowserContext::From(base::GenerateGUID(), true);
    } else {
      gin::Handle<api::Session> session;
      if (gin::ConvertFromV8(dict.isolate(), value, &session) &&
          !session.IsEmpty()) {
        browser_context = session->browser_context();
      }
    }
  }

  new URLPipeLoader(
      browser_context->GetURLLoaderFactory(), std::move(request),
      std::move(loader), std::move(client),
      static_cast<net::NetworkTrafficAnnotationTag>(traffic_annotation),
      std::move(upload_data));
}

// static
void ElectronURLLoaderFactory::StartLoadingStream(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    const gin_helper::Dictionary& dict) {
  v8::Local<v8::Value> stream;
  if (!dict.Get("data", &stream)) {
    // Assume the opts is already a stream.
    stream = dict.GetHandle();
  } else if (stream->IsNullOrUndefined()) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    // "data" was explicitly passed as null or undefined, assume the user wants
    // to send an empty body.
    //
    // Note that We must submit a empty body otherwise NetworkService would
    // crash.
    client_remote->OnReceiveResponse(std::move(head));
    mojo::ScopedDataPipeProducerHandle producer;
    mojo::ScopedDataPipeConsumerHandle consumer;
    if (mojo::CreateDataPipe(nullptr, &producer, &consumer) != MOJO_RESULT_OK) {
      client_remote->OnComplete(
          network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
      return;
    }
    producer.reset();  // The data pipe is empty.
    client_remote->OnStartLoadingResponseBody(std::move(consumer));
    client_remote->OnComplete(network::URLLoaderCompletionStatus(net::OK));
    return;
  } else if (!stream->IsObject()) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  gin_helper::Dictionary data = ToDict(dict.isolate(), stream);
  v8::Local<v8::Value> method;
  if (!data.Get("on", &method) || !method->IsFunction() ||
      !data.Get("removeListener", &method) || !method->IsFunction()) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  new NodeStreamLoader(std::move(head), std::move(loader), std::move(client),
                       data.isolate(), data.GetHandle());
}

// static
void ElectronURLLoaderFactory::SendContents(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    std::string data) {
  mojo::Remote<network::mojom::URLLoaderClient> client_remote(
      std::move(client));

  // Add header to ignore CORS.
  head->headers->AddHeader("Access-Control-Allow-Origin", "*");
  client_remote->OnReceiveResponse(std::move(head));

  // Code bellow follows the pattern of data_url_loader_factory.cc.
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  if (mojo::CreateDataPipe(nullptr, &producer, &consumer) != MOJO_RESULT_OK) {
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
    return;
  }

  client_remote->OnStartLoadingResponseBody(std::move(consumer));

  auto write_data = std::make_unique<WriteData>();
  write_data->client = std::move(client_remote);
  write_data->data = std::move(data);
  write_data->producer =
      std::make_unique<mojo::DataPipeProducer>(std::move(producer));

  base::StringPiece string_piece(write_data->data);
  write_data->producer->Write(
      std::make_unique<mojo::StringDataSource>(
          string_piece, mojo::StringDataSource::AsyncWritingMode::
                            STRING_STAYS_VALID_UNTIL_COMPLETION),
      base::BindOnce(OnWrite, std::move(write_data)));
}

}  // namespace electron
