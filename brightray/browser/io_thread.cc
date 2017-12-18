// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brightray/browser/io_thread.h"

#include "content/public/browser/browser_thread.h"
#include "net/proxy/proxy_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"

#if defined(USE_NSS_CERTS)
#include "net/cert_net/nss_ocsp.h"
#endif

using content::BrowserThread;

namespace brightray {

IOThread::IOThread() {
  BrowserThread::SetIOThreadDelegate(this);
}

IOThread::~IOThread() {
  BrowserThread::SetIOThreadDelegate(nullptr);
}

void IOThread::Init() {
  net::URLRequestContextBuilder builder;
  builder.set_proxy_service(net::ProxyService::CreateDirect());
  builder.DisableHttpCache();
  url_request_context_ = builder.Build();

#if defined(USE_NSS_CERTS)
  net::SetMessageLoopForNSSHttpIO();
  net::SetURLRequestContextForNSSHttpIO(url_request_context_.get());
#endif
}

void IOThread::CleanUp() {
#if defined(USE_NSS_CERTS)
  net::ShutdownNSSHttpIO();
  net::SetURLRequestContextForNSSHttpIO(nullptr);
#endif
  url_request_context_.reset();
}

}  // namespace brightray
