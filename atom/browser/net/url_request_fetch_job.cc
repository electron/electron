// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_fetch_job.h"

#include <algorithm>

#include "atom/browser/atom_browser_context.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "net/url_request/url_request_status.h"

namespace atom {

namespace {

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
    return job_->DataAvailable(buffer, num_bytes);
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
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const GURL& url)
    : net::URLRequestJob(request, network_delegate),
      url_(url),
      pending_buffer_size_(0) {}

void URLRequestFetchJob::HeadersCompleted() {
  response_info_.reset(new net::HttpResponseInfo);
  response_info_->headers = fetcher_->GetResponseHeaders();
  NotifyHeadersComplete();
}

int URLRequestFetchJob::DataAvailable(net::IOBuffer* buffer, int num_bytes) {
  // Clear the IO_PENDING status.
  SetStatus(net::URLRequestStatus());
  // Do nothing if pending_buffer_ is empty, i.e. there's no ReadRawData()
  // operation waiting for IO completion.
  if (!pending_buffer_.get())
    return net::ERR_IO_PENDING;;

  // pending_buffer_ is set to the IOBuffer instance provided to ReadRawData()
  // by URLRequestJob.

  int bytes_read = std::min(num_bytes, pending_buffer_size_);
  memcpy(pending_buffer_->data(), buffer->data(), bytes_read);
  NotifyReadComplete(bytes_read);
  return bytes_read;
}

void URLRequestFetchJob::Start() {
  fetcher_.reset(net::URLFetcher::Create(url_, net::URLFetcher::GET, this));
  auto context = AtomBrowserContext::Get()->url_request_context_getter();
  fetcher_->SetRequestContext(context);
  fetcher_->SaveResponseWithWriter(make_scoped_ptr(new ResponsePiper(this)));
  fetcher_->Start();
}

void URLRequestFetchJob::Kill() {
  URLRequestJob::Kill();
  fetcher_.reset();
}

bool URLRequestFetchJob::ReadRawData(net::IOBuffer* dest,
                                     int dest_size,
                                     int* bytes_read) {
  pending_buffer_ = dest;
  pending_buffer_size_ = dest_size;
  SetStatus(net::URLRequestStatus(net::URLRequestStatus::IO_PENDING, 0));
  return false;
}

bool URLRequestFetchJob::GetMimeType(std::string* mime_type) const {
  if (!response_info_)
    return false;

  return response_info_->headers->GetMimeType(mime_type);
}

void URLRequestFetchJob::GetResponseInfo(net::HttpResponseInfo* info) {
  if (response_info_)
    *info = *response_info_;
}

int URLRequestFetchJob::GetResponseCode() const {
  if (!response_info_)
    return -1;

  return response_info_->headers->response_code();
}

void URLRequestFetchJob::OnURLFetchComplete(const net::URLFetcher* source) {
  NotifyDone(fetcher_->GetStatus());
  NotifyReadComplete(0);
}

}  // namespace atom
