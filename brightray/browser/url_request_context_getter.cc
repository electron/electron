// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/url_request_context_getter.h"

#include <vector>

#include "base/task_scheduler/post_task.h"
#include "brightray/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace brightray {

class ResourceContext : public content::ResourceContext {
 public:
  ResourceContext() = default;
  ~ResourceContext() override = default;

  net::HostResolver* GetHostResolver() override {
    if (request_context_)
      return request_context_->host_resolver();
    return nullptr;
  }

  net::URLRequestContext* GetRequestContext() override {
    return request_context_;
  }

 private:
  friend class URLRequestContextGetter;

  net::URLRequestContext* request_context_;

  DISALLOW_COPY_AND_ASSIGN(ResourceContext);
};

namespace {

// For safe shutdown, must be called before
// URLRequestContextGetter::Handle is destroyed.
void NotifyContextGettersOfShutdownOnIO(
    std::unique_ptr<std::vector<scoped_refptr<URLRequestContextGetter>>>
        getters) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  for (auto& context_getter : *getters)
    context_getter->NotifyContextShuttingDown();
}

}  // namespace

URLRequestContextGetter::Handle::Handle(
    base::WeakPtr<BrowserContext> browser_context)
    : resource_context_(new ResourceContext),
      browser_context_(browser_context),
      initialized_(false) {}

URLRequestContextGetter::Handle::~Handle() {}

content::ResourceContext* URLRequestContextGetter::Handle::GetResourceContext()
    const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  LazyInitialize();
  return resource_context_.get();
}

scoped_refptr<URLRequestContextGetter>
URLRequestContextGetter::Handle::CreateMainRequestContextGetter(
    URLRequestContextGetterFactory* factory) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!main_request_context_getter_.get());
  main_request_context_getter_ =
      new URLRequestContextGetter(factory, resource_context_.get());
  return main_request_context_getter_;
}

scoped_refptr<URLRequestContextGetter>
URLRequestContextGetter::Handle::GetMediaRequestContextGetter() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return main_request_context_getter_;
}

void URLRequestContextGetter::Handle::LazyInitialize() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (initialized_)
    return;

  initialized_ = true;
  content::BrowserContext::EnsureResourceContextInitialized(
      browser_context_.get());
}

void URLRequestContextGetter::Handle::ShutdownOnUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto context_getters =
      std::make_unique<std::vector<scoped_refptr<URLRequestContextGetter>>>();
  if (media_request_context_getter_.get())
    context_getters->push_back(media_request_context_getter_);
  if (main_request_context_getter_.get())
    context_getters->push_back(main_request_context_getter_);
  if (!context_getters->empty()) {
    if (BrowserThread::IsThreadInitialized(BrowserThread::IO)) {
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::BindOnce(&NotifyContextGettersOfShutdownOnIO,
                         std::move(context_getters)));
    }
  }

  if (!BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE, this))
    delete this;
}

URLRequestContextGetter::URLRequestContextGetter(
    URLRequestContextGetterFactory* factory,
    ResourceContext* resource_context)
    : factory_(factory),
      resource_context_(resource_context),
      url_request_context_(nullptr) {
  // Must first be created on the UI thread.
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

URLRequestContextGetter::~URLRequestContextGetter() {
  DCHECK(!factory_.get());
  DCHECK(!url_request_context_);
}

void URLRequestContextGetter::NotifyContextShuttingDown() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  factory_.reset();
  url_request_context_ = nullptr;
  net::URLRequestContextGetter::NotifyContextShuttingDown();
}

net::URLRequestContext* URLRequestContextGetter::GetURLRequestContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (factory_.get() && !url_request_context_) {
    url_request_context_ = factory_->Create();
    if (resource_context_) {
      resource_context_->request_context_ = url_request_context_;
    }
  }

  return url_request_context_;
}

scoped_refptr<base::SingleThreadTaskRunner>
URLRequestContextGetter::GetNetworkTaskRunner() const {
  return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
}

}  // namespace brightray
