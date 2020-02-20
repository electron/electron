// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_
#define SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_

#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/service_worker_context_observer.h"
#include "gin/handle.h"
#include "shell/common/gin_helper/trackable_object.h"

namespace electron {

class ElectronBrowserContext;

namespace api {

class ServiceWorkerContext
    : public gin_helper::TrackableObject<ServiceWorkerContext>,
      public content::ServiceWorkerContextObserver {
 public:
  static gin::Handle<ServiceWorkerContext> Create(
      v8::Isolate* isolate,
      ElectronBrowserContext* browser_context);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  v8::Local<v8::Value> GetAllRunningWorkerInfo(v8::Isolate* isolate);
  v8::Local<v8::Value> GetWorkerInfoFromID(gin_helper::ErrorThrower thrower,
                                           int64_t version_id);

  // content::ServiceWorkerContextObserver
  void OnReportConsoleMessage(int64_t version_id,
                              const content::ConsoleMessage& message) override;
  void OnDestruct(content::ServiceWorkerContext* context) override;

 protected:
  explicit ServiceWorkerContext(v8::Isolate* isolate,
                                ElectronBrowserContext* browser_context);
  ~ServiceWorkerContext() override;

 private:
  ElectronBrowserContext* browser_context_;

  content::ServiceWorkerContext* service_worker_context_;

  base::WeakPtrFactory<ServiceWorkerContext> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerContext);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_
