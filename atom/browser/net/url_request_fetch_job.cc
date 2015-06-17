// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_fetch_job.h"

#include <string>

#include "atom/browser/atom_browser_context.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_status.h"

namespace atom {

URLRequestFetchJob::URLRequestFetchJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const GURL& url)
    : net::URLRequestJob(request, network_delegate),
      url_(url),
      weak_ptr_factory_(this) {}

URLRequestFetchJob::~URLRequestFetchJob() {}

void URLRequestFetchJob::Start() {
  net::URLFetcher* fetcher = net::URLFetcher::Create(url_,
                                                     net::URLFetcher::GET,
                                                     this);
  auto context = AtomBrowserContext::Get()->url_request_context_getter();
  fetcher->SetRequestContext(context);
  fetcher->SaveResponseWithWriter(scoped_ptr<net::URLFetcherResponseWriter>(
      this));
  fetcher->Start();
}

void URLRequestFetchJob::Kill() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  URLRequestJob::Kill();
}

bool URLRequestFetchJob::ReadRawData(net::IOBuffer* dest,
                                     int dest_size,
                                     int* bytes_read) {
  if (!dest_size) {
    *bytes_read = 0;
    return true;
  }

  int to_read = dest_size < buffer_->BytesRemaining() ?
      dest_size : buffer_->BytesRemaining();
  memcpy(dest->data(), buffer_->data(), to_read);
  buffer_->DidConsume(to_read);
  if (!buffer_->BytesRemaining()) {
    NotifyReadComplete(buffer_->size());
    return true;
  }

  *bytes_read = to_read;
  return false;
}

void URLRequestFetchJob::OnURLFetchComplete(const net::URLFetcher* source) {
  if (!source->GetStatus().is_success())
    NotifyDone(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                     source->GetResponseCode()));

  NotifyHeadersComplete();
}


int URLRequestFetchJob::Initialize(const net::CompletionCallback& callback) {
  if (buffer_)
    buffer_->Release();
  return net::OK;
}

int URLRequestFetchJob::Write(net::IOBuffer* buffer,
                              int num_bytes,
                              const net::CompletionCallback& calback) {
  buffer_ = new net::DrainableIOBuffer(buffer, num_bytes);
  set_expected_content_size(num_bytes);
  return num_bytes;
}

int URLRequestFetchJob::Finish(const net::CompletionCallback& callback) {
  return net::OK;
}

}  // namespace atom
