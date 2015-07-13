// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_session.h"

#include <string>

#include "atom/browser/api/atom_api_cookies.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "base/thread_task_runner_handle.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "native_mate/callback.h"
#include "native_mate/object_template_builder.h"
#include "net/base/load_flags.h"
#include "net/disk_cache/disk_cache.h"
#include "net/proxy/proxy_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

#include "atom/common/node_includes.h"

using content::BrowserThread;
using content::StoragePartition;

namespace {

int GetStorageMask(const std::vector<std::string>& storage_types) {
  int storage_mask = 0;

  for (auto &it : storage_types) {
    auto type = base::StringToLowerASCII(it);
    if (type == "appcache") {
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_APPCACHE;
    } else if (type == "cookies") {
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_COOKIES;
    } else if (type == "filesystem") {
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_FILE_SYSTEMS;
    } else if (type == "indexdb") {
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_INDEXEDDB;
    } else if (type == "localstorage") {
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_LOCAL_STORAGE;
    } else if (type == "shadercache") {
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_SHADER_CACHE;
    } else if (type == "websql") {
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_WEBSQL;
    } else if (type == "serviceworkers") {
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_SERVICE_WORKERS;
    }
  }

  return storage_mask;
}

int GetQuotaMask(const std::vector<std::string>& quota_types) {
  int quota_mask = 0;

  for (auto &type : quota_types) {
    if (type == "temporary") {
      quota_mask |= StoragePartition::QUOTA_MANAGED_STORAGE_MASK_TEMPORARY;
    } else if (type == "persistent") {
      quota_mask |= StoragePartition::QUOTA_MANAGED_STORAGE_MASK_PERSISTENT;
    }
  }

  return quota_mask;
}

}  // namespace

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

// Runs the callback in UI thread.
void RunCallbackInUI(const net::CompletionCallback& callback, int result) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE, base::Bind(callback, result));
}

// Callback of HttpCache::GetBackend.
void OnGetBackend(disk_cache::Backend** backend_ptr,
                  const net::CompletionCallback& callback,
                  int result) {
  if (result != net::OK) {
    RunCallbackInUI(callback, result);
  } else if (backend_ptr && *backend_ptr) {
    (*backend_ptr)->DoomAllEntries(base::Bind(&RunCallbackInUI, callback));
  } else {
    RunCallbackInUI(callback, net::ERR_FAILED);
  }
}

void ClearHttpCacheInIO(content::BrowserContext* browser_context,
                        const net::CompletionCallback& callback) {
  auto request_context =
      browser_context->GetRequestContext()->GetURLRequestContext();
  auto http_cache = request_context->http_transaction_factory()->GetCache();

  disk_cache::Backend** backend_ptr = nullptr;
  net::CompletionCallback on_get_backend =
      base::Bind(&OnGetBackend, base::Owned(backend_ptr), callback);
  int rv = http_cache->GetBackend(backend_ptr, on_get_backend);
  if (rv != net::ERR_IO_PENDING)
    on_get_backend.Run(net::OK);
}

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

void Session::ClearCache(const net::CompletionCallback& callback) {
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&ClearHttpCacheInIO,
                 base::Unretained(browser_context_),
                 callback));
}

void Session::ClearStorageData(const GURL& origin,
                               const std::vector<std::string>& storage_types,
                               const std::vector<std::string>& quota_types,
                               const base::Closure& callback) {
  auto storage_partition =
      content::BrowserContext::GetStoragePartition(browser_context_, nullptr);
  storage_partition->ClearData(
      GetStorageMask(storage_types), GetQuotaMask(quota_types), origin,
      content::StoragePartition::OriginMatcherFunction(),
      base::Time(), base::Time::Max(), callback);
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
      .SetMethod("clearCache", &Session::ClearCache)
      .SetMethod("clearStorageData", &Session::ClearStorageData)
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
