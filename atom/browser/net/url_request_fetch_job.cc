// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_fetch_job.h"

#include <algorithm>
#include <string>

#include "base/strings/string_util.h"
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
  explicit ResponsePiper(URLRequestFetchJob* job)
      : first_write_(true), job_(job) {}

  // net::URLFetcherResponseWriter:
  int Initialize(const net::CompletionCallback& callback) override {
    return net::OK;
  }
  int Write(net::IOBuffer* buffer,
            int num_bytes,
            const net::CompletionCallback& callback) override {
    if (first_write_) {
      // The URLFetcherResponseWriter doesn't have an event when headers have
      // been read, so we have to emulate by hooking to first write event.
      job_->HeadersCompleted();
      first_write_ = false;
    }
    return job_->DataAvailable(buffer, num_bytes, callback);
  }
  int Finish(const net::CompletionCallback& callback) override {
    return net::OK;
  }

 private:
  bool first_write_;
  URLRequestFetchJob* job_;

  DISALLOW_COPY_AND_ASSIGN(ResponsePiper);
};

}  // namespace

URLRequestFetchJob::URLRequestFetchJob(
    net::URLRequest* request, net::NetworkDelegate* network_delegate)
    : JsAsker<net::URLRequestJob>(request, network_delegate),
      pending_buffer_size_(0) {
}

void URLRequestFetchJob::BeforeStartInUI(
    v8::Isolate* isolate, v8::Local<v8::Value> value) {
  mate::Dictionary options;
  if (!mate::ConvertFromV8(isolate, value, &options))
    return;

  // When |session| is set to |null| we use a new request context for fetch job.
  // TODO(zcbenz): Handle the case when it is not null.
  v8::Local<v8::Value> session;
  if (options.Get("session", &session) && session->IsNull()) {
    // We have to create the URLRequestContextGetter on UI thread.
    url_request_context_getter_ = new brightray::URLRequestContextGetter(
        this, nullptr, nullptr, base::FilePath(), true,
        BrowserThread::UnsafeGetMessageLoopForThread(BrowserThread::IO),
        BrowserThread::UnsafeGetMessageLoopForThread(BrowserThread::FILE),
        nullptr, content::URLRequestInterceptorScopedVector());
  }
}

void URLRequestFetchJob::StartAsync(std::unique_ptr<base::Value> options) {
  if (!options->IsType(base::Value::TYPE_DICTIONARY)) {
    NotifyStartError(net::URLRequestStatus(
          net::URLRequestStatus::FAILED, net::ERR_NOT_IMPLEMENTED));
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
    NotifyStartError(net::URLRequestStatus(
          net::URLRequestStatus::FAILED, net::ERR_INVALID_URL));
    return;
  }

  // Use |request|'s method if |method| is not specified.
  net::URLFetcher::RequestType request_type;
  if (method.empty())
    request_type = GetRequestType(request()->method());
  else
    request_type = GetRequestType(method);

  fetcher_ = net::URLFetcher::Create(formated_url, request_type, this);
  fetcher_->SaveResponseWithWriter(make_scoped_ptr(new ResponsePiper(this)));

  // A request context getter is passed by the user.
  if (url_request_context_getter_)
    fetcher_->SetRequestContext(url_request_context_getter_.get());
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
}

void URLRequestFetchJob::HeadersCompleted() {
  response_info_.reset(new net::HttpResponseInfo);
  response_info_->headers = fetcher_->GetResponseHeaders();
  NotifyHeadersComplete();
}

int URLRequestFetchJob::DataAvailable(net::IOBuffer* buffer,
                                      int num_bytes,
                                      const net::CompletionCallback& callback) {
  // Store the data in a drainable IO buffer if pending_buffer_ is empty, i.e.
  // there's no ReadRawData() operation waiting for IO completion.
  if (!pending_buffer_.get()) {
    response_piper_callback_ = callback;
    drainable_buffer_ = new net::DrainableIOBuffer(buffer, num_bytes);
    return net::ERR_IO_PENDING;
  }

  // pending_buffer_ is set to the IOBuffer instance provided to ReadRawData()
  // by URLRequestJob.
  int bytes_read = std::min(num_bytes, pending_buffer_size_);
  memcpy(pending_buffer_->data(), buffer->data(), bytes_read);

  ClearPendingBuffer();

  ReadRawDataComplete(bytes_read);
  return bytes_read;
}

void URLRequestFetchJob::Kill() {
  JsAsker<URLRequestJob>::Kill();
  fetcher_.reset();
}

void URLRequestFetchJob::StartReading() {
  if (drainable_buffer_.get()) {
    auto num_bytes = drainable_buffer_->BytesRemaining();
    if (num_bytes <= 0 && !response_piper_callback_.is_null()) {
      int size = drainable_buffer_->BytesConsumed();
      drainable_buffer_ = nullptr;
      response_piper_callback_.Run(size);
      response_piper_callback_.Reset();
      return;
    }

    int bytes_read = std::min(num_bytes, pending_buffer_size_);
    memcpy(pending_buffer_->data(), drainable_buffer_->data(), bytes_read);

    drainable_buffer_->DidConsume(bytes_read);

    ClearPendingBuffer();

    ReadRawDataComplete(bytes_read);
  }
}

void URLRequestFetchJob::ClearPendingBuffer() {
  // Clear the buffers before notifying the read is complete, so that it is
  // safe for the observer to read.
  pending_buffer_ = nullptr;
  pending_buffer_size_ = 0;
}

int URLRequestFetchJob::ReadRawData(net::IOBuffer* dest, int dest_size) {
  if (GetResponseCode() == 204) {
    request()->set_received_response_content_length(prefilter_bytes_read());
    return net::OK;
  }
  pending_buffer_ = dest;
  pending_buffer_size_ = dest_size;
  // Read from drainable_buffer_ if available, i.e. response data became
  // available before ReadRawData was called.
  StartReading();
  return net::ERR_IO_PENDING;
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
  if (!response_info_) {
    // Since we notify header completion only after first write there will be
    // no response object constructed for http respones with no content 204.
    // We notify header completion here.
    HeadersCompleted();
    return;
  }

  ClearPendingBuffer();

  if (fetcher_->GetStatus().is_success())
    ReadRawDataComplete(0);
  else
    NotifyStartError(fetcher_->GetStatus());
}

}  // namespace atom
