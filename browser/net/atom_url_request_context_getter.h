// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_URL_REQUEST_CONTEXT_GETTER_H_
#define ATOM_BROWSER_NET_ATOM_URL_REQUEST_CONTEXT_GETTER_H_

#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/content_browser_client.h"
#include "net/url_request/url_request_context_getter.h"

namespace base {
class MessageLoop;
}

namespace brightray {
class NetworkDelegate;
}

namespace net {
class HostResolver;
class ProxyConfigService;
class URLRequestContextStorage;
}

namespace atom {

class AtomURLRequestJobFactory;

class AtomURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  AtomURLRequestContextGetter(
      const base::FilePath& base_path,
      base::MessageLoop* io_loop,
      base::MessageLoop* file_loop,
      scoped_ptr<brightray::NetworkDelegate> network_delegate,
      content::ProtocolHandlerMap* protocol_handlers);

  // net::URLRequestContextGetter implementations:
  virtual net::URLRequestContext* GetURLRequestContext() OVERRIDE;
  virtual scoped_refptr<base::SingleThreadTaskRunner>
      GetNetworkTaskRunner() const OVERRIDE;

  net::HostResolver* host_resolver();
  net::URLRequestContextStorage* storage() const { return storage_.get(); }
  AtomURLRequestJobFactory* job_factory() const { return job_factory_; }

 protected:
  virtual ~AtomURLRequestContextGetter();

 private:
  base::FilePath base_path_;
  base::MessageLoop* io_loop_;
  base::MessageLoop* file_loop_;
  AtomURLRequestJobFactory* job_factory_;

  scoped_ptr<net::ProxyConfigService> proxy_config_service_;
  scoped_ptr<brightray::NetworkDelegate> network_delegate_;
  scoped_ptr<net::URLRequestContextStorage> storage_;
  scoped_ptr<net::URLRequestContext> url_request_context_;
  content::ProtocolHandlerMap protocol_handlers_;

  DISALLOW_COPY_AND_ASSIGN(AtomURLRequestContextGetter);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_URL_REQUEST_CONTEXT_GETTER_H_
