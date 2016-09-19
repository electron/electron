// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_url_request.h"
#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/atom_browser_context.h"
#include "base/callback.h"
#include "content/public/browser/browser_thread.h"

namespace atom {

AtomURLRequest::AtomURLRequest(base::WeakPtr<api::URLRequest> delegate) 
  : delegate_(delegate) {
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
  // post to io thread
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






int AtomURLRequest::StatusCode() {
  return url_request_->GetResponseCode();
}

void AtomURLRequest::StatusMessage() {
}
void AtomURLRequest::ResponseHeaders() {
}

void AtomURLRequest::ResponseHttpVersion() {
}



void AtomURLRequest::StartOnIOThread() {
  url_request_->Start();
}


void AtomURLRequest::set_method(const std::string& method) {
  url_request_->set_method(method);
}

void AtomURLRequest::OnResponseStarted(net::URLRequest* request)
{
  // post to main thread
  content::BrowserThread::PostTask(
    content::BrowserThread::UI, FROM_HERE,
    base::Bind(&AtomURLRequest::InformDelegeteResponseStarted, this));
}

void AtomURLRequest::OnReadCompleted(net::URLRequest* request, int bytes_read)
{
  // post to main thread
}

void AtomURLRequest::InformDelegeteResponseStarted() {
  if (delegate_) {
    delegate_->OnResponseStarted();
  }
}


}  // namespace atom