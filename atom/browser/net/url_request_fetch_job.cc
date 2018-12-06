// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_fetch_job.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/dictionary.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"

using content::BrowserThread;

namespace atom {

namespace {

// Convert string to RequestType.
net::URLFetcher::RequestType GetRequestType(const std::string& raw) {
  std::string method = base::ToUpperASCII(raw);
  if (method.empty() || method == "GET")
    return net::URLFetcher::GET;
  else if (method == "POST")
    return net::URLFetcher::POST;
  else if (method == "HEAD")
    return net::URLFetcher::HEAD;
  else if (method == "DELETE")
    return net::URLFetcher::DELETE_REQUEST;
  else if (method == "PUT")
    return net::URLFetcher::PUT;
  else if (method == "PATCH")
    return net::URLFetcher::PATCH;
  else  // Use "GET" as fallback.
    return net::URLFetcher::GET;
}

// Pipe the response writer back to URLRequestFetchJob.
class ResponsePiper : public net::URLFetcherResponseWriter {
 public:
  explicit ResponsePiper(URLRequestFetchJob* job) : job_(job) {}

  // net::URLFetcherResponseWriter:
  int Initialize(net::CompletionOnceCallback callback) override {
    return net::OK;
  }
  int Write(net::IOBuffer* buffer,
            int num_bytes,
            net::CompletionOnceCallback callback) override {
    if (first_write_) {
      // The URLFetcherResponseWriter doesn't have an event when headers have
      // been read, so we have to emulate by hooking to first write event.
      job_->HeadersCompleted();
      first_write_ = false;
    }
    return job_->DataAvailable(buffer, num_bytes, std::move(callback));
  }
  int Finish(int net_error, net::CompletionOnceCallback callback) override {
    return net::OK;
  }

 private:
  bool first_write_ = true;
  URLRequestFetchJob* job_;

  DISALLOW_COPY_AND_ASSIGN(ResponsePiper);
};

void BeforeStartInUI(base::WeakPtr<URLRequestFetchJob> job,
                     mate::Arguments* args) {
  // Pass whatever user passed to the actaul request job.
  v8::Local<v8::Value> value;
  mate::Dictionary options;
  if (!args->GetNext(&value) ||
      !mate::ConvertFromV8(args->isolate(), value, &options)) {
    base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
                             base::BindOnce(&URLRequestFetchJob::OnError, job,
                                            net::ERR_NOT_IMPLEMENTED));
    return;
  }

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter;
  scoped_refptr<AtomBrowserContext> custom_browser_context;
  // When |session| is set to |null| we use a new request context for fetch
  // job.
  if (options.Get("session", &value)) {
    if (value->IsNull()) {
      // We have to create the URLRequestContextGetter on UI thread.
      custom_browser_context =
          AtomBrowserContext::From(base::GenerateGUID(), true);
      url_request_context_getter = custom_browser_context->GetRequestContext();
    } else {
      mate::Handle<api::Session> session;
      if (mate::ConvertFromV8(args->isolate(), value, &session) &&
          !session.IsEmpty()) {
        AtomBrowserContext* browser_context = session->browser_context();
        url_request_context_getter = browser_context->GetRequestContext();
      }
    }
  }

  V8ValueConverter converter;
  v8::Local<v8::Context> context = args->isolate()->GetCurrentContext();
  std::unique_ptr<base::Value> request_options(
      converter.FromV8Value(value, context));

  int error = net::OK;
  if (!request_options || !request_options->is_dict())
    error = net::ERR_NOT_IMPLEMENTED;

  JsAsker::IsErrorOptions(request_options.get(), &error);

  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&URLRequestFetchJob::StartAsync, job,
                     base::RetainedRef(url_request_context_getter),
                     base::RetainedRef(custom_browser_context),
                     std::move(request_options), error));
}

}  // namespace

URLRequestFetchJob::URLRequestFetchJob(net::URLRequest* request,
                                       net::NetworkDelegate* network_delegate)
    : net::URLRequestJob(request, network_delegate), weak_factory_(this) {}

URLRequestFetchJob::~URLRequestFetchJob() = default;

void URLRequestFetchJob::Start() {
  auto request_details = std::make_unique<base::DictionaryValue>();
  FillRequestDetails(request_details.get(), request());
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&JsAsker::AskForOptions, base::Unretained(isolate()),
                     handler(), std::move(request_details),
                     base::Bind(&BeforeStartInUI, weak_factory_.GetWeakPtr())));
}

void URLRequestFetchJob::StartAsync(
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
    scoped_refptr<AtomBrowserContext> browser_context,
    std::unique_ptr<base::Value> options,
    int error) {
  if (error != net::OK) {
    NotifyStartError(
        net::URLRequestStatus(net::URLRequestStatus::FAILED, error));
    return;
  }

  std::string url, method, referrer;
  base::DictionaryValue* upload_data = nullptr;
  base::DictionaryValue* dict =
      static_cast<base::DictionaryValue*>(options.get());
  dict->GetString("url", &url);
  dict->GetString("method", &method);
  dict->GetString("referrer", &referrer);
  dict->GetDictionary("uploadData", &upload_data);

  // Check if URL is valid.
  GURL formated_url(url);
  if (!formated_url.is_valid()) {
    NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                           net::ERR_INVALID_URL));
    return;
  }

  // Use |request|'s method if |method| is not specified.
  net::URLFetcher::RequestType request_type;
  if (method.empty())
    request_type = GetRequestType(request()->method());
  else
    request_type = GetRequestType(method);

  fetcher_ = net::URLFetcher::Create(formated_url, request_type, this);
  fetcher_->SaveResponseWithWriter(base::WrapUnique(new ResponsePiper(this)));

  // A request context getter is passed by the user.
  if (url_request_context_getter)
    fetcher_->SetRequestContext(url_request_context_getter.get());
  else
    fetcher_->SetRequestContext(request_context_getter());

  // Use |request|'s referrer if |referrer| is not specified.
  if (referrer.empty())
    fetcher_->SetReferrer(request()->referrer());
  else
    fetcher_->SetReferrer(referrer);

  // Set the data needed for POSTs.
  if (upload_data && request_type == net::URLFetcher::POST) {
    std::string content_type, data;
    upload_data->GetString("contentType", &content_type);
    upload_data->GetString("data", &data);
    fetcher_->SetUploadData(content_type, data);
  }

  // Use |request|'s headers.
  fetcher_->SetExtraRequestHeaders(
      request()->extra_request_headers().ToString());

  fetcher_->Start();

  if (browser_context)
    custom_browser_context_ = browser_context;
}

void URLRequestFetchJob::OnError(int error) {
  NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED, error));
}

void URLRequestFetchJob::HeadersCompleted() {
  response_info_.reset(new net::HttpResponseInfo);
  response_info_->headers = fetcher_->GetResponseHeaders();
  NotifyHeadersComplete();
}

int URLRequestFetchJob::DataAvailable(net::IOBuffer* buffer,
                                      int num_bytes,
                                      net::CompletionOnceCallback callback) {
  // When pending_buffer_ is empty, there's no ReadRawData() operation waiting
  // for IO completion, we have to save the parameters until the request is
  // ready to read data.
  if (!pending_buffer_.get()) {
    write_buffer_ = buffer;
    write_num_bytes_ = num_bytes;
    write_callback_ = std::move(callback);
    return net::ERR_IO_PENDING;
  }

  // Write data to the pending buffer and clear them after the writing.
  int bytes_read = BufferCopy(buffer, num_bytes, pending_buffer_.get(),
                              pending_buffer_size_);
  ClearPendingBuffer();
  ReadRawDataComplete(bytes_read);
  return bytes_read;
}

void URLRequestFetchJob::Kill() {
  weak_factory_.InvalidateWeakPtrs();
  net::URLRequestJob::Kill();
  fetcher_.reset();
  custom_browser_context_ = nullptr;
}

int URLRequestFetchJob::ReadRawData(net::IOBuffer* dest, int dest_size) {
  if (GetResponseCode() == 204) {
    request()->set_received_response_content_length(prefilter_bytes_read());
    return net::OK;
  }

  // When write_buffer_ is empty, there is no data valable yet, we have to save
  // the dest buffer util DataAvailable.
  if (!write_buffer_.get()) {
    pending_buffer_ = dest;
    pending_buffer_size_ = dest_size;
    return net::ERR_IO_PENDING;
  }

  // Read from the write buffer and clear them after reading.
  int bytes_read =
      BufferCopy(write_buffer_.get(), write_num_bytes_, dest, dest_size);
  ClearWriteBuffer();
  if (!write_callback_.is_null())
    std::move(write_callback_).Run(bytes_read);
  return bytes_read;
}

bool URLRequestFetchJob::GetMimeType(std::string* mime_type) const {
  if (!response_info_ || !response_info_->headers)
    return false;

  return response_info_->headers->GetMimeType(mime_type);
}

void URLRequestFetchJob::GetResponseInfo(net::HttpResponseInfo* info) {
  if (response_info_)
    *info = *response_info_;
}

int URLRequestFetchJob::GetResponseCode() const {
  if (!response_info_ || !response_info_->headers)
    return -1;

  return response_info_->headers->response_code();
}

void URLRequestFetchJob::OnURLFetchComplete(const net::URLFetcher* source) {
  ClearPendingBuffer();
  ClearWriteBuffer();

  if (fetcher_->GetStatus().is_success()) {
    if (!response_info_) {
      // Since we notify header completion only after first write there will be
      // no response object constructed for http respones with no content 204.
      // We notify header completion here.
      HeadersCompleted();
      return;
    }
    if (request_->status().is_io_pending()) {
      ReadRawDataComplete(0);
    }
  } else {
    NotifyStartError(fetcher_->GetStatus());
  }
}

int URLRequestFetchJob::BufferCopy(net::IOBuffer* source,
                                   int num_bytes,
                                   net::IOBuffer* target,
                                   int target_size) {
  int bytes_written = std::min(num_bytes, target_size);
  memcpy(target->data(), source->data(), bytes_written);
  return bytes_written;
}

void URLRequestFetchJob::ClearPendingBuffer() {
  pending_buffer_ = nullptr;
  pending_buffer_size_ = 0;
}

void URLRequestFetchJob::ClearWriteBuffer() {
  write_buffer_ = nullptr;
  write_num_bytes_ = 0;
}

}  // namespace atom
