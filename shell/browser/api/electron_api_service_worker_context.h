// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/service_worker_context_observer.h"
#include "content/public/browser/storage_partition_config.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "third_party/blink/public/common/service_worker/embedded_worker_status.h"
#include "third_party/blink/public/common/tokens/tokens.h"

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
    : public gin::Wrappable<ServiceWorkerContext>,
      public gin_helper::EventEmitterMixin<ServiceWorkerContext>,
      private content::ServiceWorkerContextObserver {
 public:
  static ServiceWorkerContext* Create(v8::Isolate* isolate,
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
      content::ChildProcessId process_id,
      int thread_id,
      const blink::ServiceWorkerToken& token);
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

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  static const char* GetClassName() { return "ServiceWorkerContext"; }
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  void Trace(cppgc::Visitor*) const override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

  // disable copy
  ServiceWorkerContext(const ServiceWorkerContext&) = delete;
  ServiceWorkerContext& operator=(const ServiceWorkerContext&) = delete;

  explicit ServiceWorkerContext(v8::Isolate* isolate,
                                ElectronBrowserContext* browser_context);
  ~ServiceWorkerContext() override;

 private:
  void OnRunningStatusChanged(int64_t version_id,
                              blink::EmbeddedWorkerStatus running_status);

  raw_ptr<content::ServiceWorkerContext> service_worker_context_;

  // A key identifying the owning BrowserContext.
  // Used in ServiceWorkerMain lookups.
  const std::string browser_context_id_;

  // A key identifying a StoragePartition within a BrowserContext.
  // Used in ServiceWorkerMain lookups.
  const content::StoragePartitionConfig storage_partition_config_;

  gin::WeakCellFactory<ServiceWorkerContext> weak_factory_{this};
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_
