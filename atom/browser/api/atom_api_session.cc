// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_session.h"

#include <string>

#include "atom/browser/api/atom_api_cookies.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "base/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "native_mate/callback.h"
#include "native_mate/object_template_builder.h"
#include "net/base/load_flags.h"
#include "net/proxy/proxy_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

class ResolveProxyHelper {
 public:
  ResolveProxyHelper(AtomBrowserContext* browser_context,
                     const GURL& url,
                     Session::ResolveProxyCallback callback)
      : callback_(callback),
        original_thread_(base::ThreadTaskRunnerHandle::Get()) {
    scoped_refptr<net::URLRequestContextGetter> context_getter =
        browser_context->GetRequestContext();
    context_getter->GetNetworkTaskRunner()->PostTask(
        FROM_HERE,
        base::Bind(&ResolveProxyHelper::ResolveProxy,
                   base::Unretained(this), context_getter, url));
  }

  void OnResolveProxyCompleted(int result) {
    std::string proxy;
    if (result == net::OK)
      proxy = proxy_info_.ToPacString();
    original_thread_->PostTask(FROM_HERE,
                               base::Bind(callback_, proxy));
    delete this;
  }

 private:
  void ResolveProxy(scoped_refptr<net::URLRequestContextGetter> context_getter,
                    const GURL& url) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

    net::ProxyService* proxy_service =
        context_getter->GetURLRequestContext()->proxy_service();
    net::CompletionCallback completion_callback =
        base::Bind(&ResolveProxyHelper::OnResolveProxyCompleted,
                   base::Unretained(this));

    // Start the request.
    int result = proxy_service->ResolveProxy(
        url, net::LOAD_NORMAL, &proxy_info_, completion_callback,
        &pac_req_, nullptr, net::BoundNetLog());

    // Completed synchronously.
    if (result != net::ERR_IO_PENDING)
      completion_callback.Run(result);
  }

  Session::ResolveProxyCallback callback_;
  net::ProxyInfo proxy_info_;
  net::ProxyService::PacRequest* pac_req_;
  scoped_refptr<base::SingleThreadTaskRunner> original_thread_;

  DISALLOW_COPY_AND_ASSIGN(ResolveProxyHelper);
};

}  // namespace

Session::Session(AtomBrowserContext* browser_context)
    : browser_context_(browser_context) {
  AttachAsUserData(browser_context);
}

Session::~Session() {
}

void Session::ResolveProxy(const GURL& url, ResolveProxyCallback callback) {
  new ResolveProxyHelper(browser_context_, url, callback);
}

v8::Local<v8::Value> Session::Cookies(v8::Isolate* isolate) {
  if (cookies_.IsEmpty()) {
    auto handle = atom::api::Cookies::Create(isolate, browser_context_);
    cookies_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, cookies_);
}

mate::ObjectTemplateBuilder Session::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("resolveProxy", &Session::ResolveProxy)
      .SetProperty("cookies", &Session::Cookies);
}

// static
mate::Handle<Session> Session::CreateFrom(
    v8::Isolate* isolate,
    AtomBrowserContext* browser_context) {
  auto existing = TrackableObject::FromWrappedClass(isolate, browser_context);
  if (existing)
    return mate::CreateHandle(isolate, static_cast<Session*>(existing));

  return mate::CreateHandle(isolate, new Session(browser_context));
}

}  // namespace api

}  // namespace atom
