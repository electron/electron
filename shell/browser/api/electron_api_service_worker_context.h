// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_

#include "base/memory/raw_ptr.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/service_worker_context_observer.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/wrappable.h"
#include "third_party/blink/public/common/service_worker/embedded_worker_status.h"

namespace content {
class StoragePartition;
}

namespace gin_helper {
template <typename T>
class Handle;
template <typename T>
class Promise;
}  // namespace gin_helper

namespace electron {

class ElectronBrowserContext;

namespace api {

class ServiceWorkerMain;

class ServiceWorkerContext final
    : public gin_helper::DeprecatedWrappable<ServiceWorkerContext>,
      public gin_helper::EventEmitterMixin<ServiceWorkerContext>,
      private content::ServiceWorkerContextObserver {
 public:
  static gin_helper::Handle<ServiceWorkerContext> Create(
      v8::Isolate* isolate,
      ElectronBrowserContext* browser_context);

  v8::Local<v8::Value> GetAllRunningWorkerInfo(v8::Isolate* isolate);
  v8::Local<v8::Value> GetInfoFromVersionID(gin_helper::ErrorThrower thrower,
                                            int64_t version_id);
  v8::Local<v8::Value> GetFromVersionID(gin_helper::ErrorThrower thrower,
                                        int64_t version_id);
  v8::Local<v8::Value> GetWorkerFromVersionID(v8::Isolate* isolate,
                                              int64_t version_id);
  gin_helper::Handle<ServiceWorkerMain> GetWorkerFromVersionIDIfExists(
      v8::Isolate* isolate,
      int64_t version_id);
  v8::Local<v8::Promise> StartWorkerForScope(v8::Isolate* isolate, GURL scope);
  void DidStartWorkerForScope(
      std::shared_ptr<gin_helper::Promise<v8::Local<v8::Value>>> shared_promise,
      int64_t version_id,
      int process_id,
      int thread_id);
  void DidFailToStartWorkerForScope(
      std::shared_ptr<gin_helper::Promise<v8::Local<v8::Value>>> shared_promise,
      content::StatusCodeResponse status);
  void StopWorkersForScope(GURL scope);
  v8::Local<v8::Promise> StopAllWorkers(v8::Isolate* isolate);

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

  // gin_helper::Wrappable
  static gin::DeprecatedWrapperInfo kWrapperInfo;
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
