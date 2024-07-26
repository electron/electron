// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/electron_url_loader_factory.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/uuid.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "mojo/public/cpp/system/string_data_source.h"
#include "net/base/filename_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/redirect_util.h"
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
#include "shell/common/gin_helper/dictionary.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

#include "shell/common/node_includes.h"

using content::BrowserThread;

namespace gin {

template <>
struct Converter<electron::ProtocolType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::ProtocolType* out) {
    using Val = electron::ProtocolType;
    static constexpr auto Lookup =
        base::MakeFixedFlatMap<std::string_view, Val>({
            // note "free" is internal type, not allowed to be passed from user
            {"buffer", Val::kBuffer},
            {"file", Val::kFile},
            {"http", Val::kHttp},
            {"stream", Val::kStream},
            {"string", Val::kString},
        });
    return FromV8WithLookup(isolate, val, Lookup, out);
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

bool LooksLikeStream(v8::Isolate* isolate, v8::Local<v8::Value> v) {
  // the stream loader can handle null and undefined as "empty body". Could
  // probably be more efficient here but this works.
  if (v->IsNullOrUndefined())
    return true;
  if (!v->IsObject())
    return false;
  gin_helper::Dictionary dict(isolate, v.As<v8::Object>());
  v8::Local<v8::Value> method;
  return dict.Get("on", &method) && method->IsFunction() &&
         dict.Get("removeListener", &method) && method->IsFunction();
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
    head->headers =
        base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
    return head;
  }

  int status_code = net::HTTP_OK;
  dict.Get("statusCode", &status_code);
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      base::StringPrintf("HTTP/1.1 %d %s", status_code,
                         net::GetHttpReasonPhrase(
                             static_cast<net::HttpStatusCode>(status_code))));

  dict.Get("charset", &head->charset);
  bool has_mime_type = dict.Get("mimeType", &head->mime_type);
  bool has_content_type = false;

  base::Value::Dict headers;
  if (dict.Get("headers", &headers)) {
    for (const auto iter : headers) {
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
      auto header_name_lowercase = base::ToLowerASCII(iter.first);

      if (header_name_lowercase == "content-type") {
        // Some apps are passing content-type via headers, which is not accepted
        // in NetworkService.
        head->headers->GetMimeTypeAndCharset(&head->mime_type, &head->charset);
        has_content_type = true;
      } else if (header_name_lowercase == "content-length" &&
                 iter.second.is_string()) {
        base::StringToInt64(iter.second.GetString(), &head->content_length);
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
  network::URLLoaderCompletionStatus status(net::ERR_FAILED);
  if (result == MOJO_RESULT_OK) {
    status = network::URLLoaderCompletionStatus(net::OK);
    status.encoded_data_length = write_data->data.size();
    status.encoded_body_length = write_data->data.size();
    status.decoded_body_length = write_data->data.size();
  }
  write_data->client->OnComplete(status);
}

}  // namespace

ElectronURLLoaderFactory::RedirectedRequest::RedirectedRequest(
    const net::RedirectInfo& redirect_info,
    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory_remote)
    : redirect_info_(redirect_info),
      request_id_(request_id),
      options_(options),
      request_(request),
      client_(std::move(client)),
      traffic_annotation_(traffic_annotation) {
  loader_receiver_.Bind(std::move(loader_receiver));
  loader_receiver_.set_disconnect_handler(
      base::BindOnce(&ElectronURLLoaderFactory::RedirectedRequest::DeleteThis,
                     base::Unretained(this)));
  target_factory_remote_.Bind(std::move(target_factory_remote));
  target_factory_remote_.set_disconnect_handler(base::BindOnce(
      &ElectronURLLoaderFactory::RedirectedRequest::OnTargetFactoryError,
      base::Unretained(this)));
}

ElectronURLLoaderFactory::RedirectedRequest::~RedirectedRequest() = default;

void ElectronURLLoaderFactory::RedirectedRequest::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const std::optional<GURL>& new_url) {
  // Update |request_| with info from the redirect, so that it's accurate
  // The following references code in WorkerScriptLoader::FollowRedirect
  bool should_clear_upload = false;
  net::RedirectUtil::UpdateHttpRequest(
      request_.url, request_.method, redirect_info_, removed_headers,
      modified_headers, &request_.headers, &should_clear_upload);
  request_.cors_exempt_headers.MergeFrom(modified_cors_exempt_headers);
  for (const std::string& name : removed_headers)
    request_.cors_exempt_headers.RemoveHeader(name);

  if (should_clear_upload)
    request_.request_body = nullptr;

  request_.url = redirect_info_.new_url;
  request_.method = redirect_info_.new_method;
  request_.site_for_cookies = redirect_info_.new_site_for_cookies;
  request_.referrer = GURL(redirect_info_.new_referrer);
  request_.referrer_policy = redirect_info_.new_referrer_policy;

  // Create a new loader to process the redirect and destroy this one
  target_factory_remote_->CreateLoaderAndStart(
      loader_receiver_.Unbind(), request_id_, options_, request_,
      std::move(client_), traffic_annotation_);

  DeleteThis();
}

void ElectronURLLoaderFactory::RedirectedRequest::OnTargetFactoryError() {
  // Can't create a new loader at this point, so the request can't continue
  mojo::Remote<network::mojom::URLLoaderClient> client_remote(
      std::move(client_));
  client_remote->OnComplete(
      network::URLLoaderCompletionStatus(net::ERR_FAILED));
  client_remote.reset();

  DeleteThis();
}

void ElectronURLLoaderFactory::RedirectedRequest::DeleteThis() {
  loader_receiver_.reset();
  target_factory_remote_.reset();

  delete this;
}

// static
mojo::PendingRemote<network::mojom::URLLoaderFactory>
ElectronURLLoaderFactory::Create(ProtocolType type,
                                 const ProtocolHandler& handler) {
  mojo::PendingRemote<network::mojom::URLLoaderFactory> pending_remote;

  // The ElectronURLLoaderFactory will delete itself when there are no more
  // receivers - see the SelfDeletingURLLoaderFactory::OnDisconnect method.
  new ElectronURLLoaderFactory(type, handler,
                               pending_remote.InitWithNewPipeAndPassReceiver());

  return pending_remote;
}

ElectronURLLoaderFactory::ElectronURLLoaderFactory(
    ProtocolType type,
    const ProtocolHandler& handler,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver)
    : network::SelfDeletingURLLoaderFactory(std::move(factory_receiver)),
      type_(type),
      handler_(handler) {}

ElectronURLLoaderFactory::~ElectronURLLoaderFactory() = default;

void ElectronURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // |StartLoading| is used for both intercepted and registered protocols,
  // and on redirects it needs a factory to use to create a loader for the
  // new request. So in this case, this factory is the target factory.
  mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory;
  this->Clone(target_factory.InitWithNewPipeAndPassReceiver());

  handler_.Run(
      request,
      base::BindOnce(&ElectronURLLoaderFactory::StartLoading, std::move(loader),
                     request_id, options, request, std::move(client),
                     traffic_annotation, std::move(target_factory), type_));
}

// static
void ElectronURLLoaderFactory::OnComplete(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    int32_t request_id,
    const network::URLLoaderCompletionStatus& status) {
  if (client.is_valid()) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(status);
  }
}

// static
void ElectronURLLoaderFactory::StartLoading(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
    ProtocolType type,
    gin::Arguments* args) {
  // Send network error when there is no argument passed.
  //
  // Note that we should not throw JS error in the callback no matter what is
  // passed, to keep compatibility with old code.
  v8::Local<v8::Value> response;
  if (!args->GetNext(&response)) {
    OnComplete(std::move(client), request_id,
               network::URLLoaderCompletionStatus(net::ERR_NOT_IMPLEMENTED));
    return;
  }

  // Parse {error} object.
  gin_helper::Dictionary dict = ToDict(args->isolate(), response);
  if (!dict.IsEmpty()) {
    int error_code;
    if (dict.Get("error", &error_code)) {
      OnComplete(std::move(client), request_id,
                 network::URLLoaderCompletionStatus(error_code));
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
    // If the request is a MAIN_FRAME request, the first-party URL gets
    // updated on redirects.
    const net::RedirectInfo::FirstPartyURLPolicy first_party_url_policy =
        request.resource_type ==
                static_cast<int>(blink::mojom::ResourceType::kMainFrame)
            ? net::RedirectInfo::FirstPartyURLPolicy::UPDATE_URL_ON_REDIRECT
            : net::RedirectInfo::FirstPartyURLPolicy::NEVER_CHANGE_URL;

    net::RedirectInfo redirect_info = net::RedirectInfo::ComputeRedirectInfo(
        request.method, request.url, request.site_for_cookies,
        first_party_url_policy, request.referrer_policy,
        request.referrer.GetAsReferrer().spec(), head->headers->response_code(),
        request.url.Resolve(location),
        net::RedirectUtil::GetReferrerPolicyHeader(head->headers.get()), false);

    DCHECK(client.is_valid());

    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));

    client_remote->OnReceiveRedirect(redirect_info, std::move(head));

    // Bind the URLLoader receiver and wait for a FollowRedirect request, or for
    // the remote to disconnect, which will happen if the request is aborted.
    // That may happen when the redirect is to a different scheme, which will
    // cause the URL loader to be destroyed and a new one created using the
    // factory for that scheme.
    new RedirectedRequest(redirect_info, std::move(loader), request_id, options,
                          request, client_remote.Unbind(), traffic_annotation,
                          std::move(target_factory));

    return;
  }

  // Some protocol accepts non-object responses.
  if (dict.IsEmpty() && ResponseMustBeObject(type)) {
    OnComplete(std::move(client), request_id,
               network::URLLoaderCompletionStatus(net::ERR_NOT_IMPLEMENTED));
    return;
  }

  switch (type) {
    // DEPRECATED: Soon only |kFree| will be supported!
    case ProtocolType::kBuffer:
      if (response->IsArrayBufferView())
        StartLoadingBuffer(std::move(client), std::move(head),
                           response.As<v8::ArrayBufferView>());
      else if (v8::Local<v8::Value> data; !dict.IsEmpty() &&
                                          dict.Get("data", &data) &&
                                          data->IsArrayBufferView())
        StartLoadingBuffer(std::move(client), std::move(head),
                           data.As<v8::ArrayBufferView>());
      else
        OnComplete(std::move(client), request_id,
                   network::URLLoaderCompletionStatus(net::ERR_FAILED));
      break;
    case ProtocolType::kString: {
      std::string data;
      if (gin::ConvertFromV8(args->isolate(), response, &data))
        SendContents(std::move(client), std::move(head), data);
      else if (!dict.IsEmpty() && dict.Get("data", &data))
        SendContents(std::move(client), std::move(head), data);
      else
        OnComplete(std::move(client), request_id,
                   network::URLLoaderCompletionStatus(net::ERR_FAILED));
      break;
    }
    case ProtocolType::kFile: {
      base::FilePath path;
      if (gin::ConvertFromV8(args->isolate(), response, &path))
        StartLoadingFile(std::move(client), std::move(loader), std::move(head),
                         request, path, dict);
      else if (!dict.IsEmpty() && dict.Get("path", &path))
        StartLoadingFile(std::move(client), std::move(loader), std::move(head),
                         request, path, dict);
      else
        OnComplete(std::move(client), request_id,
                   network::URLLoaderCompletionStatus(net::ERR_FAILED));
      break;
    }
    case ProtocolType::kHttp:
      if (GURL url; !dict.IsEmpty() && dict.Get("url", &url) && url.is_valid())
        StartLoadingHttp(std::move(client), std::move(loader), request,
                         traffic_annotation, dict);
      else
        OnComplete(std::move(client), request_id,
                   network::URLLoaderCompletionStatus(net::ERR_FAILED));
      break;
    case ProtocolType::kStream:
      StartLoadingStream(std::move(client), std::move(loader), std::move(head),
                         dict);
      break;

    case ProtocolType::kFree: {
      // Infer the type based on the object given
      v8::Local<v8::Value> data;
      if (!dict.IsEmpty() && dict.Has("data"))
        dict.Get("data", &data);
      else
        data = response;

      // |data| can be either a string, a buffer or a stream.
      if (data->IsArrayBufferView()) {
        StartLoadingBuffer(std::move(client), std::move(head),
                           data.As<v8::ArrayBufferView>());
      } else if (data->IsString()) {
        SendContents(std::move(client), std::move(head),
                     gin::V8ToString(args->isolate(), data));
      } else if (LooksLikeStream(args->isolate(), data)) {
        StartLoadingStream(std::move(client), std::move(loader),
                           std::move(head), dict);
      } else if (!dict.IsEmpty()) {
        // |data| wasn't specified, so look for |response.url| or
        // |response.path|.
        if (GURL url; dict.Get("url", &url))
          StartLoadingHttp(std::move(client), std::move(loader), request,
                           traffic_annotation, dict);
        else if (base::FilePath path; dict.Get("path", &path))
          StartLoadingFile(std::move(client), std::move(loader),
                           std::move(head), request, path, dict);
        else
          // Don't know what kind of response this is, so fail.
          OnComplete(std::move(client), request_id,
                     network::URLLoaderCompletionStatus(net::ERR_FAILED));
      } else {
        OnComplete(std::move(client), request_id,
                   network::URLLoaderCompletionStatus(net::ERR_FAILED));
      }
      break;
    }
  }
}

// static
void ElectronURLLoaderFactory::StartLoadingBuffer(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    v8::Local<v8::ArrayBufferView> buffer) {
  SendContents(std::move(client), std::move(head),
               std::string(node::Buffer::Data(buffer.As<v8::Value>()),
                           node::Buffer::Length(buffer.As<v8::Value>())));
}

// static
void ElectronURLLoaderFactory::StartLoadingFile(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    network::mojom::URLResponseHeadPtr head,
    const network::ResourceRequest& original_request,
    const base::FilePath& path,
    const gin_helper::Dictionary& opts) {
  network::ResourceRequest request = original_request;
  request.url = net::FilePathToFileURL(path);
  if (!opts.IsEmpty()) {
    opts.Get("referrer", &request.referrer);
    opts.Get("method", &request.method);
  }

  // Add header to ignore CORS.
  head->headers->AddHeader("Access-Control-Allow-Origin", "*");
  asar::CreateAsarURLLoader(request, std::move(loader), std::move(client),
                            head->headers);
}

// static
void ElectronURLLoaderFactory::StartLoadingHttp(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    const network::ResourceRequest& original_request,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    const gin_helper::Dictionary& dict) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->headers = original_request.headers;
  request->cors_exempt_headers = original_request.cors_exempt_headers;

  dict.Get("url", &request->url);
  dict.Get("referrer", &request->referrer);
  if (!dict.Get("method", &request->method))
    request->method = original_request.method;

  base::Value::Dict upload_data;
  if (request->method != net::HttpRequestHeaders::kGetMethod &&
      request->method != net::HttpRequestHeaders::kHeadMethod)
    dict.Get("uploadData", &upload_data);

  ElectronBrowserContext* browser_context =
      ElectronBrowserContext::From("", false);
  v8::Local<v8::Value> value;
  if (dict.Get("session", &value)) {
    if (value->IsNull()) {
      browser_context = ElectronBrowserContext::From(
          base::Uuid::GenerateRandomV4().AsLowercaseString(), true);
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
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    network::mojom::URLResponseHeadPtr head,
    const gin_helper::Dictionary& dict) {
  v8::Local<v8::Value> stream;
  if (!dict.Get("data", &stream)) {
    // Assume the opts is already a stream.
    stream = dict.GetHandle();
  } else if (stream->IsNullOrUndefined()) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    mojo::ScopedDataPipeProducerHandle producer;
    mojo::ScopedDataPipeConsumerHandle consumer;
    if (mojo::CreateDataPipe(nullptr, producer, consumer) != MOJO_RESULT_OK) {
      client_remote->OnComplete(
          network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
      return;
    }
    // "data" was explicitly passed as null or undefined, assume the user wants
    // to send an empty body.
    //
    // Note that We must submit a empty body otherwise NetworkService would
    // crash.
    client_remote->OnReceiveResponse(std::move(head), std::move(consumer),
                                     std::nullopt);
    producer.reset();  // The data pipe is empty.
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

  // Code below follows the pattern of data_url_loader_factory.cc.
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  if (mojo::CreateDataPipe(nullptr, producer, consumer) != MOJO_RESULT_OK) {
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
    return;
  }

  client_remote->OnReceiveResponse(std::move(head), std::move(consumer),
                                   std::nullopt);

  auto write_data = std::make_unique<WriteData>();
  write_data->client = std::move(client_remote);
  write_data->data = std::move(data);
  write_data->producer =
      std::make_unique<mojo::DataPipeProducer>(std::move(producer));
  auto* producer_ptr = write_data->producer.get();

  std::string_view string_view(write_data->data);
  producer_ptr->Write(
      std::make_unique<mojo::StringDataSource>(
          string_view, mojo::StringDataSource::AsyncWritingMode::
                           STRING_STAYS_VALID_UNTIL_COMPLETION),
      base::BindOnce(OnWrite, std::move(write_data)));
}

}  // namespace electron
