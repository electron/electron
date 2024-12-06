// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_

#include "base/memory/raw_ptr.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/service_worker_context_observer.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "third_party/blink/public/common/service_worker/embedded_worker_status.h"

namespace content {
class StoragePartition;
}

namespace gin {
template <typename T>
class Handle;
}  // namespace gin

namespace electron {

class ElectronBrowserContext;

namespace api {

class ServiceWorkerMain;

class ServiceWorkerContext final
    : public gin::Wrappable<ServiceWorkerContext>,
      public gin_helper::EventEmitterMixin<ServiceWorkerContext>,
      private content::ServiceWorkerContextObserver {
 public:
  static gin::Handle<ServiceWorkerContext> Create(
      v8::Isolate* isolate,
      ElectronBrowserContext* browser_context);

  v8::Local<v8::Value> GetAllRunningWorkerInfo(v8::Isolate* isolate);
  v8::Local<v8::Value> GetInfoFromVersionID(gin_helper::ErrorThrower thrower,
                                            int64_t version_id);
  v8::Local<v8::Value> GetFromVersionID(gin_helper::ErrorThrower thrower,
                                        int64_t version_id);
  v8::Local<v8::Value> GetWorkerFromVersionID(gin_helper::ErrorThrower thrower,
                                              int64_t version_id);
  gin::Handle<ServiceWorkerMain> GetWorkerFromVersionIDIfExists(
      v8::Isolate* isolate,
      int64_t version_id);

  // content::ServiceWorkerContextObserver
  void OnReportConsoleMessage(int64_t version_id,
                              const GURL& scope,
                              const content::ConsoleMessage& message) override;
  void OnRegistrationCompleted(const GURL& scope) override;
  void OnVersionStartingRunning(int64_t version_id) override;
  void OnVersionStartedRunning(
      int64_t version_id,
      const content::ServiceWorkerRunningInfo& running_info) override;
  void OnVersionStoppingRunning(int64_t version_id) override;
  void OnVersionStoppedRunning(int64_t version_id) override;
  void OnVersionRedundant(int64_t version_id, const GURL& scope) override;
  void OnDestruct(content::ServiceWorkerContext* context) override;

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // disable copy
  ServiceWorkerContext(const ServiceWorkerContext&) = delete;
  ServiceWorkerContext& operator=(const ServiceWorkerContext&) = delete;

 protected:
  explicit ServiceWorkerContext(v8::Isolate* isolate,
                                ElectronBrowserContext* browser_context);
  ~ServiceWorkerContext() override;

 private:
  void OnRunningStatusChanged(int64_t version_id,
                              blink::EmbeddedWorkerStatus running_status);

  raw_ptr<content::ServiceWorkerContext> service_worker_context_;

  // Service worker registration and versions are unique to a storage partition.
  // Keep a reference to the storage partition to be used for lookups.
  raw_ptr<content::StoragePartition> storage_partition_;

  base::WeakPtrFactory<ServiceWorkerContext> weak_ptr_factory_{this};
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_
