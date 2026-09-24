// Copyright (c) 2025 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_service_worker_main.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/no_destructor.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"  // nogncheck
#include "content/browser/service_worker/service_worker_info.h"     // nogncheck
#include "content/browser/service_worker/service_worker_version.h"  // nogncheck
#include "gin/object_template_builder.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/api/message_port.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/serialized_value_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_util.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-cppgc.h"

namespace {

// Use private API to get the live version of the service worker. This will
// exist while in starting, stopping, or stopped running status.
content::ServiceWorkerVersion* GetLiveVersion(
    content::ServiceWorkerContext* service_worker_context,
    int64_t version_id) {
  auto* wrapper = static_cast<content::ServiceWorkerContextWrapper*>(
      service_worker_context);
  return wrapper->GetLiveVersion(version_id);
}

// Get a public ServiceWorkerVersionBaseInfo object directly from the service
// worker.
std::optional<content::ServiceWorkerVersionBaseInfo> GetLiveVersionInfo(
    content::ServiceWorkerContext* service_worker_context,
    int64_t version_id) {
  auto* version = GetLiveVersion(service_worker_context, version_id);
  if (version) {
    return version->GetInfo();
  }
  return std::nullopt;
}

}  // namespace

namespace electron::api {

// ServiceWorkerKey -> ServiceWorkerMain
auto& GetVersionIdMap() {
  using Map = base::flat_map<ServiceWorkerKey,
                             cppgc::WeakPersistent<ServiceWorkerMain>>;
  static base::NoDestructor<Map> instance;
  return *instance;
}

ServiceWorkerMain* FromServiceWorkerKey(const ServiceWorkerKey& key) {
  return base::FindPtrOrNull(GetVersionIdMap(), key);
}

// static
ServiceWorkerMain* ServiceWorkerMain::FromVersionID(
    std::string browser_context_id,
    content::StoragePartitionConfig storage_partition_config,
    int64_t version_id) {
  const ServiceWorkerKey key{std::move(browser_context_id),
                             std::move(storage_partition_config), version_id};
  return FromServiceWorkerKey(key);
}

gin::WrapperInfo ServiceWorkerMain::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronServiceWorkerMain);

ServiceWorkerMain::ServiceWorkerMain(content::ServiceWorkerContext* sw_context,
                                     int64_t version_id,
                                     ServiceWorkerKey key)
    : version_id_{version_id},
      key_{std::move(key)},
      service_worker_context_{sw_context} {
  GetVersionIdMap().emplace(key_, this);
  InvalidateVersionInfo();
}

ServiceWorkerMain::~ServiceWorkerMain() {
  Destroy();
}

void ServiceWorkerMain::Destroy() {
  if (version_destroyed_)
    return;
  version_destroyed_ = true;
  InvalidateVersionInfo();
  MaybeDisconnectRemote();
  GetVersionIdMap().erase(key_);
  keep_alive_.Clear();
}

void ServiceWorkerMain::MaybeDisconnectRemote() {
  if (remote_.is_bound() &&
      (version_destroyed_ ||
       (!service_worker_context_->IsLiveStartingServiceWorker(version_id_) &&
        !service_worker_context_->IsLiveRunningServiceWorker(version_id_)))) {
    remote_.reset();
  }
}

mojom::ElectronRenderer* ServiceWorkerMain::GetRendererApi() {
  if (!remote_.is_bound()) {
    if (!service_worker_context_->IsLiveRunningServiceWorker(version_id_)) {
      return nullptr;
    }

    service_worker_context_->GetRemoteAssociatedInterfaces(version_id_)
        .GetInterface(&remote_);
  }
  return remote_.get();
}

void ServiceWorkerMain::Send(gin::Arguments* args) {
  std::string channel;
  electron::SerializedValue message;
  if (!ReadIPCSendArguments(args, "ServiceWorkerMain", &channel, &message))
    return;

  auto* renderer_api_remote = GetRendererApi();
  if (!renderer_api_remote) {
    return;
  }

  renderer_api_remote->Message(false, channel, std::move(message));
}

v8::Local<v8::Value> ServiceWorkerMain::StartTask(v8::Isolate* isolate) {
  gin_helper::ErrorThrower thrower(isolate);
  if (version_destroyed_) {
    thrower.ThrowTypeError("ServiceWorkerMain is destroyed");
    return {};
  }

  // TODO(samuelmaddock): maybe make timeout configurable in the future
  auto request_uuid = base::Uuid::GenerateRandomV4();
  content::ServiceWorkerExternalRequestResult start_result =
      service_worker_context_->StartingExternalRequest(
          version_id_,
          content::ServiceWorkerExternalRequestTimeoutType::kDoesNotTimeout,
          request_uuid);
  if (start_result != content::ServiceWorkerExternalRequestResult::kOk) {
    thrower.ThrowError("Unable to start service worker task.");
    return {};
  }

  auto task = gin_helper::Dictionary::CreateEmpty(isolate);
  // The task references this ServiceWorkerMain so that end() can be called for
  // as long as the task is alive; end() itself only holds it weakly.
  v8::Local<v8::Object> wrapper;
  if (GetWrapper(isolate).ToLocal(&wrapper)) {
    task.GetHandle()
        ->SetPrivate(isolate->GetCurrentContext(),
                     v8::Private::ForApi(
                         isolate, gin::StringToV8(isolate, "serviceWorker")),
                     wrapper)
        .Check();
  }
  task.Set(
      "end",
      gin::ConvertToV8(
          isolate, base::BindRepeating(
                       [](const cppgc::WeakPersistent<ServiceWorkerMain>& self,
                          const std::string& uuid, v8::Isolate* isolate) {
                         if (self)
                           self->FinishExternalRequest(isolate, uuid);
                       },
                       cppgc::WeakPersistent<ServiceWorkerMain>(this),
                       request_uuid.AsLowercaseString())));
  return task.GetHandle();
}

void ServiceWorkerMain::InvalidateVersionInfo() {
  version_info_.reset();

  if (version_destroyed_)
    return;

  version_info_ = GetLiveVersionInfo(service_worker_context_, version_id_);

  // if there's no version info, mark the version as destroyed
  if (!version_info_)
    Destroy();
}

void ServiceWorkerMain::OnRunningStatusChanged(
    blink::EmbeddedWorkerStatus running_status) {
  // Disconnect remote when content::ServiceWorkerHost has terminated.
  MaybeDisconnectRemote();

  InvalidateVersionInfo();

  // Redundant worker has been marked for deletion. Now that it's stopped, let's
  // destroy our wrapper.
  if (redundant_ && running_status == blink::EmbeddedWorkerStatus::kStopped) {
    Destroy();
  }
}

void ServiceWorkerMain::OnVersionRedundant() {
  // Redundant service workers have been either unregistered or replaced. A new
  // ServiceWorkerMain will need to be created.
  // Set internal state to mark it for deletion once it has fully stopped.
  redundant_ = true;

  // content only broadcasts OnStopped for versions that reached RUNNING;
  // anything else gets no further running status change, so destroy now.
  auto* version = GetLiveVersion(service_worker_context_, version_id_);
  if (!version ||
      version->running_status() != blink::EmbeddedWorkerStatus::kRunning) {
    Destroy();
  }
}

bool ServiceWorkerMain::IsDestroyed() const {
  return version_destroyed_;
}

const blink::StorageKey ServiceWorkerMain::GetStorageKey() {
  const GURL& scope = version_info_ ? version_info()->scope : GURL::EmptyGURL();
  return blink::StorageKey::CreateFirstParty(url::Origin::Create(scope));
}

void ServiceWorkerMain::FinishExternalRequest(v8::Isolate* isolate,
                                              std::string uuid) {
  base::Uuid request_uuid = base::Uuid::ParseLowercase(uuid);
  if (!request_uuid.is_valid()) {
    isolate->ThrowException(v8::Exception::TypeError(
        gin::StringToV8(isolate, "Invalid external request UUID")));
    return;
  }

  DCHECK(service_worker_context_);
  if (!service_worker_context_)
    return;

  content::ServiceWorkerExternalRequestResult result =
      service_worker_context_->FinishedExternalRequest(version_id_,
                                                       request_uuid);
  // Still release the request above so a doomed worker can stop.
  if (version_destroyed_) {
    isolate->ThrowException(v8::Exception::TypeError(
        gin::StringToV8(isolate, "ServiceWorkerMain is destroyed")));
    return;
  }

  std::string error;
  switch (result) {
    case content::ServiceWorkerExternalRequestResult::kOk:
      break;
    case content::ServiceWorkerExternalRequestResult::kBadRequestId:
      error = "Unknown external request UUID";
      break;
    case content::ServiceWorkerExternalRequestResult::kWorkerNotRunning:
      error = "Service worker is no longer running";
      break;
    case content::ServiceWorkerExternalRequestResult::kWorkerNotFound:
      error = "Service worker was not found";
      break;
    case content::ServiceWorkerExternalRequestResult::kNullContext:
    default:
      error = "Service worker context is unavailable and may be shutting down";
      break;
  }

  if (!error.empty()) {
    isolate->ThrowException(
        v8::Exception::TypeError(gin::StringToV8(isolate, error)));
  }
}

size_t ServiceWorkerMain::CountExternalRequestsForTest() {
  if (version_destroyed_)
    return 0;
  auto& storage_key = GetStorageKey();
  return service_worker_context_->CountExternalRequestsForTest(storage_key);
}

int64_t ServiceWorkerMain::VersionID() const {
  return version_id_;
}

GURL ServiceWorkerMain::ScopeURL() const {
  if (version_destroyed_)
    return {};
  return version_info()->scope;
}

GURL ServiceWorkerMain::ScriptURL() const {
  if (version_destroyed_)
    return {};
  return version_info()->script_url;
}

// static
ServiceWorkerMain* ServiceWorkerMain::New(v8::Isolate* isolate) {
  return nullptr;
}

// static
ServiceWorkerMain* ServiceWorkerMain::From(
    v8::Isolate* isolate,
    content::ServiceWorkerContext* sw_context,
    std::string browser_context_id,
    content::StoragePartitionConfig storage_partition_config,
    int64_t version_id) {
  ServiceWorkerKey service_worker_key{std::move(browser_context_id),
                                      std::move(storage_partition_config),
                                      version_id};

  if (auto* service_worker = FromServiceWorkerKey(service_worker_key))
    return service_worker;

  // Ensure ServiceWorkerVersion exists and is not redundant (pending deletion)
  auto* live_version = GetLiveVersion(sw_context, version_id);
  if (!live_version || live_version->is_redundant()) {
    return nullptr;
  }

  return cppgc::MakeGarbageCollected<ServiceWorkerMain>(
      isolate->GetCppHeap()->GetAllocationHandle(), sw_context, version_id,
      std::move(service_worker_key));
}

// static
void ServiceWorkerMain::FillObjectTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  gin_helper::ObjectTemplateBuilder(isolate, templ)
      .SetMethod("send", &ServiceWorkerMain::Send)
      .SetMethod("startTask", &ServiceWorkerMain::StartTask)
      .SetMethod("isDestroyed", &ServiceWorkerMain::IsDestroyed)
      .SetMethod("_countExternalRequests",
                 &ServiceWorkerMain::CountExternalRequestsForTest)
      .SetProperty("versionId", &ServiceWorkerMain::VersionID)
      .SetProperty("scope", &ServiceWorkerMain::ScopeURL)
      .SetProperty("scriptURL", &ServiceWorkerMain::ScriptURL)
      .Build();
}

const gin::WrapperInfo* ServiceWorkerMain::wrapper_info() const {
  return &kWrapperInfo;
}

const char* ServiceWorkerMain::GetHumanReadableName() const {
  return "Electron / ServiceWorkerMain";
}

}  // namespace electron::api

namespace {

using electron::api::ServiceWorkerMain;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("ServiceWorkerMain",
           ServiceWorkerMain::GetConstructor(isolate, context,
                                             &ServiceWorkerMain::kWrapperInfo));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_service_worker_main,
                                  Initialize)
