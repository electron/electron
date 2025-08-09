// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_service_worker_context.h"

#include <string_view>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/console_message.h"
#include "content/public/browser/storage_partition.h"
#include "gin/data_object_builder.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/electron_api_service_worker_main.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/service_worker_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_util.h"

using ServiceWorkerStatus =
    content::ServiceWorkerRunningInfo::ServiceWorkerVersionStatus;

namespace electron::api {

namespace {

constexpr std::string_view MessageSourceToString(
    const blink::mojom::ConsoleMessageSource source) {
  switch (source) {
    case blink::mojom::ConsoleMessageSource::kXml:
      return "xml";
    case blink::mojom::ConsoleMessageSource::kJavaScript:
      return "javascript";
    case blink::mojom::ConsoleMessageSource::kNetwork:
      return "network";
    case blink::mojom::ConsoleMessageSource::kConsoleApi:
      return "console-api";
    case blink::mojom::ConsoleMessageSource::kStorage:
      return "storage";
    case blink::mojom::ConsoleMessageSource::kRendering:
      return "rendering";
    case blink::mojom::ConsoleMessageSource::kSecurity:
      return "security";
    case blink::mojom::ConsoleMessageSource::kDeprecation:
      return "deprecation";
    case blink::mojom::ConsoleMessageSource::kWorker:
      return "worker";
    case blink::mojom::ConsoleMessageSource::kViolation:
      return "violation";
    case blink::mojom::ConsoleMessageSource::kIntervention:
      return "intervention";
    case blink::mojom::ConsoleMessageSource::kRecommendation:
      return "recommendation";
    default:
      return "other";
  }
}

v8::Local<v8::Value> ServiceWorkerRunningInfoToDict(
    v8::Isolate* isolate,
    const content::ServiceWorkerRunningInfo& info) {
  return gin::DataObjectBuilder(isolate)
      .Set("scriptUrl", info.script_url.spec())
      .Set("scope", info.scope.spec())
      .Set("renderProcessId", info.render_process_id)
      .Build();
}

}  // namespace

gin::DeprecatedWrapperInfo ServiceWorkerContext::kWrapperInfo = {
    gin::kEmbedderNativeGin};

ServiceWorkerContext::ServiceWorkerContext(
    v8::Isolate* isolate,
    ElectronBrowserContext* browser_context) {
  storage_partition_ = browser_context->GetDefaultStoragePartition();
  service_worker_context_ = storage_partition_->GetServiceWorkerContext();
  service_worker_context_->AddObserver(this);
}

ServiceWorkerContext::~ServiceWorkerContext() {
  service_worker_context_->RemoveObserver(this);
}

void ServiceWorkerContext::OnRunningStatusChanged(
    int64_t version_id,
    blink::EmbeddedWorkerStatus running_status) {
  ServiceWorkerMain* worker =
      ServiceWorkerMain::FromVersionID(version_id, storage_partition_);
  if (worker)
    worker->OnRunningStatusChanged(running_status);

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent("running-status-changed",
                   gin::DataObjectBuilder(isolate)
                       .Set("versionId", version_id)
                       .Set("runningStatus", running_status)
                       .Build());
}

void ServiceWorkerContext::OnReportConsoleMessage(
    int64_t version_id,
    const GURL& scope,
    const content::ConsoleMessage& message) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  Emit("console-message",
       gin::DataObjectBuilder(isolate)
           .Set("versionId", version_id)
           .Set("source", MessageSourceToString(message.source))
           .Set("level", static_cast<int32_t>(message.message_level))
           .Set("message", message.message)
           .Set("lineNumber", message.line_number)
           .Set("sourceUrl", message.source_url.spec())
           .Build());
}

void ServiceWorkerContext::OnRegistrationCompleted(const GURL& scope) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  Emit("registration-completed",
       gin::DataObjectBuilder(isolate).Set("scope", scope).Build());
}

void ServiceWorkerContext::OnVersionRedundant(int64_t version_id,
                                              const GURL& scope) {
  ServiceWorkerMain* worker =
      ServiceWorkerMain::FromVersionID(version_id, storage_partition_);
  if (worker)
    worker->OnVersionRedundant();
}

void ServiceWorkerContext::OnVersionStartingRunning(int64_t version_id) {
  OnRunningStatusChanged(version_id, blink::EmbeddedWorkerStatus::kStarting);
}

void ServiceWorkerContext::OnVersionStartedRunning(
    int64_t version_id,
    const content::ServiceWorkerRunningInfo& running_info) {
  OnRunningStatusChanged(version_id, blink::EmbeddedWorkerStatus::kRunning);
}

void ServiceWorkerContext::OnVersionStoppingRunning(int64_t version_id) {
  OnRunningStatusChanged(version_id, blink::EmbeddedWorkerStatus::kStopping);
}

void ServiceWorkerContext::OnVersionStoppedRunning(int64_t version_id) {
  OnRunningStatusChanged(version_id, blink::EmbeddedWorkerStatus::kStopped);
}

void ServiceWorkerContext::OnDestruct(content::ServiceWorkerContext* context) {
  if (context == service_worker_context_) {
    delete this;
  }
}

v8::Local<v8::Value> ServiceWorkerContext::GetAllRunningWorkerInfo(
    v8::Isolate* isolate) {
  gin::DataObjectBuilder builder(isolate);
  const base::flat_map<int64_t, content::ServiceWorkerRunningInfo>& info_map =
      service_worker_context_->GetRunningServiceWorkerInfos();
  for (const auto& iter : info_map) {
    builder.Set(
        base::NumberToString(iter.first),
        ServiceWorkerRunningInfoToDict(isolate, std::move(iter.second)));
  }
  return builder.Build();
}

v8::Local<v8::Value> ServiceWorkerContext::GetInfoFromVersionID(
    gin_helper::ErrorThrower thrower,
    int64_t version_id) {
  const base::flat_map<int64_t, content::ServiceWorkerRunningInfo>& info_map =
      service_worker_context_->GetRunningServiceWorkerInfos();
  auto iter = info_map.find(version_id);
  if (iter == info_map.end()) {
    thrower.ThrowError("Could not find service worker with that version_id");
    return {};
  }
  return ServiceWorkerRunningInfoToDict(thrower.isolate(),
                                        std::move(iter->second));
}

v8::Local<v8::Value> ServiceWorkerContext::GetFromVersionID(
    gin_helper::ErrorThrower thrower,
    int64_t version_id) {
  util::EmitWarning(thrower.isolate(),
                    "The session.serviceWorkers.getFromVersionID API is "
                    "deprecated, use "
                    "session.serviceWorkers.getInfoFromVersionID instead.",
                    "ServiceWorkersDeprecateGetFromVersionID");

  return GetInfoFromVersionID(thrower, version_id);
}

v8::Local<v8::Value> ServiceWorkerContext::GetWorkerFromVersionID(
    v8::Isolate* isolate,
    int64_t version_id) {
  return ServiceWorkerMain::From(isolate, service_worker_context_,
                                 storage_partition_, version_id)
      .ToV8();
}

gin_helper::Handle<ServiceWorkerMain>
ServiceWorkerContext::GetWorkerFromVersionIDIfExists(v8::Isolate* isolate,
                                                     int64_t version_id) {
  ServiceWorkerMain* worker =
      ServiceWorkerMain::FromVersionID(version_id, storage_partition_);
  if (!worker)
    return gin_helper::Handle<ServiceWorkerMain>();
  return gin_helper::CreateHandle(isolate, worker);
}

v8::Local<v8::Promise> ServiceWorkerContext::StartWorkerForScope(
    v8::Isolate* isolate,
    GURL scope) {
  auto shared_promise =
      std::make_shared<gin_helper::Promise<v8::Local<v8::Value>>>(isolate);
  v8::Local<v8::Promise> handle = shared_promise->GetHandle();

  blink::StorageKey storage_key =
      blink::StorageKey::CreateFirstParty(url::Origin::Create(scope));
  service_worker_context_->StartWorkerForScope(
      scope, storage_key,
      base::BindOnce(&ServiceWorkerContext::DidStartWorkerForScope,
                     weak_ptr_factory_.GetWeakPtr(), shared_promise),
      base::BindOnce(&ServiceWorkerContext::DidFailToStartWorkerForScope,
                     weak_ptr_factory_.GetWeakPtr(), shared_promise));

  return handle;
}

void ServiceWorkerContext::DidStartWorkerForScope(
    std::shared_ptr<gin_helper::Promise<v8::Local<v8::Value>>> shared_promise,
    int64_t version_id,
    int process_id,
    int thread_id) {
  v8::Isolate* isolate = shared_promise->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Value> service_worker_main =
      GetWorkerFromVersionID(isolate, version_id);
  shared_promise->Resolve(service_worker_main);
  shared_promise.reset();
}

void ServiceWorkerContext::DidFailToStartWorkerForScope(
    std::shared_ptr<gin_helper::Promise<v8::Local<v8::Value>>> shared_promise,
    content::StatusCodeResponse status) {
  shared_promise->RejectWithErrorMessage("Failed to start service worker.");
  shared_promise.reset();
}

v8::Local<v8::Promise> ServiceWorkerContext::StopAllWorkers(
    v8::Isolate* isolate) {
  auto promise = gin_helper::Promise<void>(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  service_worker_context_->StopAllServiceWorkers(base::BindOnce(
      [](gin_helper::Promise<void> promise) { promise.Resolve(); },
      std::move(promise)));

  return handle;
}

// static
gin_helper::Handle<ServiceWorkerContext> ServiceWorkerContext::Create(
    v8::Isolate* isolate,
    ElectronBrowserContext* browser_context) {
  return gin_helper::CreateHandle(
      isolate, new ServiceWorkerContext(isolate, browser_context));
}

// static
gin::ObjectTemplateBuilder ServiceWorkerContext::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<
             ServiceWorkerContext>::GetObjectTemplateBuilder(isolate)
      .SetMethod("getAllRunning",
                 &ServiceWorkerContext::GetAllRunningWorkerInfo)
      .SetMethod("getFromVersionID", &ServiceWorkerContext::GetFromVersionID)
      .SetMethod("getInfoFromVersionID",
                 &ServiceWorkerContext::GetInfoFromVersionID)
      .SetMethod("getWorkerFromVersionID",
                 &ServiceWorkerContext::GetWorkerFromVersionID)
      .SetMethod("_getWorkerFromVersionIDIfExists",
                 &ServiceWorkerContext::GetWorkerFromVersionIDIfExists)
      .SetMethod("startWorkerForScope",
                 &ServiceWorkerContext::StartWorkerForScope)
      .SetMethod("_stopAllWorkers", &ServiceWorkerContext::StopAllWorkers);
}

const char* ServiceWorkerContext::GetTypeName() {
  return "ServiceWorkerContext";
}

}  // namespace electron::api
