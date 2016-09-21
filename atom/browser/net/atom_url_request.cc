// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_url_request.h"
#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/atom_browser_context.h"
#include "base/callback.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"

namespace {

const int kBufferSize = 4096;

} // namespace

namespace atom {

AtomURLRequest::AtomURLRequest(base::WeakPtr<api::URLRequest> delegate) 
  : delegate_(delegate)
  , buffer_( new net::IOBuffer(kBufferSize)) {
}

AtomURLRequest::~AtomURLRequest() {
}

scoped_refptr<AtomURLRequest> AtomURLRequest::create(
  AtomBrowserContext* browser_context,
  const std::string& url,
  base::WeakPtr<api::URLRequest> delegate) {

  DCHECK(browser_context);
  DCHECK(!url.empty());

  auto request_context_getter = browser_context->url_request_context_getter();

  DCHECK(request_context_getter);

  auto context = request_context_getter->GetURLRequestContext();

  DCHECK(context);

  scoped_refptr<AtomURLRequest> atom_url_request = new AtomURLRequest(delegate);

  atom_url_request->url_request_ = context->CreateRequest(GURL(url),
    net::RequestPriority::DEFAULT_PRIORITY,
    atom_url_request.get());

  return atom_url_request;

}

void AtomURLRequest::Write() {
}

void AtomURLRequest::End() {
  // Called on content::BrowserThread::UI
  content::BrowserThread::PostTask(
    content::BrowserThread::IO, FROM_HERE,
    base::Bind(&AtomURLRequest::StartOnIOThread, this));
}

void AtomURLRequest::Abort() {
}

void AtomURLRequest::SetHeader() {

}

void AtomURLRequest::GetHeader() {

}

void AtomURLRequest::RemoveHeader() {

}



scoped_refptr<net::HttpResponseHeaders> AtomURLRequest::GetResponseHeaders() {
  return url_request_->response_headers();
}



void AtomURLRequest::StartOnIOThread() {
  // Called on content::BrowserThread::IO

  url_request_->Start();
}


void AtomURLRequest::set_method(const std::string& method) {
  url_request_->set_method(method);
}

void AtomURLRequest::OnResponseStarted(net::URLRequest* request) {
  // Called on content::BrowserThread::IO

  DCHECK_EQ(request, url_request_.get());

  if (url_request_->status().is_success()) {
    // Cache net::HttpResponseHeaders instance, a read-only objects
    // so that headers and other http metainformation can be simultaneously
    // read from UI thread while request data is simulataneously streaming
    // on IO thread.
    response_headers_ = url_request_->response_headers();
  }

  content::BrowserThread::PostTask(
    content::BrowserThread::UI, FROM_HERE,
    base::Bind(&AtomURLRequest::InformDelegateResponseStarted, this));

  ReadResponse();
}

void AtomURLRequest::ReadResponse() {

  // Called on content::BrowserThread::IO

  // Some servers may treat HEAD requests as GET requests. To free up the
  // network connection as soon as possible, signal that the request has
  // completed immediately, without trying to read any data back (all we care
  // about is the response code and headers, which we already have).
  int bytes_read = 0;
  if (url_request_->status().is_success() /* TODO && (request_type_ != URLFetcher::HEAD)*/) {
    if (!url_request_->Read(buffer_.get(), kBufferSize, &bytes_read))
      bytes_read = -1; 
  }
  OnReadCompleted(url_request_.get(), bytes_read);
}


void AtomURLRequest::OnReadCompleted(net::URLRequest* request, int bytes_read) {
  // Called on content::BrowserThread::IO

  DCHECK_EQ(request, url_request_.get());

  do {
    if (!url_request_->status().is_success() || bytes_read <= 0)
      break;


    const auto result = CopyAndPostBuffer(bytes_read);
    if (!result) {
      // Failed to transfer data to UI thread.
      return;
    }
  } while (url_request_->Read(buffer_.get(), kBufferSize, &bytes_read));
    
  const auto status = url_request_->status();

  if (!status.is_io_pending() /* TODO || request_type_ == URLFetcher::HEAD*/ ) {

    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&AtomURLRequest::InformDelegateResponseCompleted, this));
  }

}


bool AtomURLRequest::CopyAndPostBuffer(int bytes_read) {
  // Called on content::BrowserThread::IO.

  // data is only a wrapper for the async buffer_.
  // Make a deep copy of payload and transfer ownership to the UI thread.
  scoped_refptr<net::IOBufferWithSize> buffer_copy(new net::IOBufferWithSize(bytes_read));
  memcpy(buffer_copy->data(), buffer_->data(), bytes_read);

  return content::BrowserThread::PostTask(
    content::BrowserThread::UI, FROM_HERE,
    base::Bind(&AtomURLRequest::InformDelegateResponseData, this, buffer_copy));
}


void AtomURLRequest::InformDelegateResponseStarted() {
  // Called  on content::BrowserThread::UI.

  if (delegate_) {
    delegate_->OnResponseStarted();
  }
}

void AtomURLRequest::InformDelegateResponseData(scoped_refptr<net::IOBufferWithSize> data) {
  // Called  on content::BrowserThread::IO.

  // Transfer ownership of the data buffer, data will be released
  // by the delegate's OnResponseData.
  if (delegate_) {
    delegate_->OnResponseData(data);
  }
}

void AtomURLRequest::InformDelegateResponseCompleted() {
  if (delegate_) {
    delegate_->OnResponseCompleted();
  }
}


}  // namespace atom