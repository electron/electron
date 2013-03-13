// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_URL_REQUEST_CONTEXT_GETTER_H_
#define BRIGHTRAY_BROWSER_URL_REQUEST_CONTEXT_GETTER_H_

#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/content_browser_client.h"
#include "net/url_request/url_request_context_getter.h"

class MessageLoop;

namespace net {
class HostResolver;
class NetworkDelegate;
class ProxyConfigService;
class URLRequestContextStorage;
}

namespace brightray {

class URLRequestContextGetter : public net::URLRequestContextGetter {
public:
  URLRequestContextGetter(
      const base::FilePath& base_path,
      MessageLoop* io_loop,
      MessageLoop* file_loop,
      content::ProtocolHandlerMap*);
  virtual ~URLRequestContextGetter();

  net::HostResolver* host_resolver();
  virtual net::URLRequestContext* GetURLRequestContext() OVERRIDE;

private:
  virtual scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner() const OVERRIDE;

  base::FilePath base_path_;
  MessageLoop* io_loop_;
  MessageLoop* file_loop_;

  scoped_ptr<net::ProxyConfigService> proxy_config_service_;
  scoped_ptr<net::NetworkDelegate> network_delegate_;
  scoped_ptr<net::URLRequestContextStorage> storage_;
  scoped_ptr<net::URLRequestContext> url_request_context_;
  content::ProtocolHandlerMap protocol_handlers_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestContextGetter);
};

}

#endif
