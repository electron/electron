// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_service_worker_context.h"

#include "chrome/browser/browser_process.h"
#include "content/public/browser/console_message.h"
#include "content/public/browser/storage_partition.h"
#include "native_mate/converter.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/common/native_mate_converters/value_converter.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

namespace {

std::string MessageSourceToString(
    const blink::mojom::ConsoleMessageSource source) {
  if (source == blink::mojom::ConsoleMessageSource::kXml)
    return "xml";
  if (source == blink::mojom::ConsoleMessageSource::kJavaScript)
    return "javascript";
  if (source == blink::mojom::ConsoleMessageSource::kNetwork)
    return "network";
  if (source == blink::mojom::ConsoleMessageSource::kConsoleApi)
    return "console-api";
  if (source == blink::mojom::ConsoleMessageSource::kStorage)
    return "storage";
  if (source == blink::mojom::ConsoleMessageSource::kAppCache)
    return "app-cache";
  if (source == blink::mojom::ConsoleMessageSource::kRendering)
    return "rendering";
  if (source == blink::mojom::ConsoleMessageSource::kSecurity)
    return "security";
  if (source == blink::mojom::ConsoleMessageSource::kDeprecation)
    return "deprecation";
  if (source == blink::mojom::ConsoleMessageSource::kWorker)
    return "worker";
  if (source == blink::mojom::ConsoleMessageSource::kViolation)
    return "violation";
  if (source == blink::mojom::ConsoleMessageSource::kIntervention)
    return "intervention";
  if (source == blink::mojom::ConsoleMessageSource::kRecommendation)
    return "recommendation";
  return "other";
}

// base::DictionaryValue ServiceWorkerRunningInfoToDict(const
// content::ServiceWorkerRunningInfo& info) {
//   base::DictionaryValue dict;
//   return dict;
// }

}  // namespace

ServiceWorkerContext::ServiceWorkerContext(v8::Isolate* isolate,
                                           AtomBrowserContext* browser_context)
    : browser_context_(browser_context), weak_ptr_factory_(this) {
  Init(isolate);
  service_worker_context_ =
      content::BrowserContext::GetDefaultStoragePartition(browser_context_)
          ->GetServiceWorkerContext();
  service_worker_context_->AddObserver(this);
}

ServiceWorkerContext::~ServiceWorkerContext() {
  service_worker_context_->RemoveObserver(this);
}

void ServiceWorkerContext::OnReportConsoleMessage(
    int64_t version_id,
    const content::ConsoleMessage& message) {
  base::DictionaryValue details;
  details.SetDoublePath("versionId", static_cast<double>(version_id));
  details.SetStringPath("source", MessageSourceToString(message.source));
  details.SetIntPath("level", static_cast<int32_t>(message.message_level));
  details.SetStringPath("message", message.message);
  details.SetIntPath("lineNumber", message.line_number);
  details.SetStringPath("sourceUrl", message.source_url.spec());
  Emit("console-message", details);
}

// mate::Dictionary GetAllWorkers() {

// }

// base::DictionaryValue
// ServiceWorkerContext::GetWorkerInfoFromID(gin_helper::ErrorThrower thrower,
// int64_t version_id) {
//   auto info_map = service_worker_context_->GetRunningServiceWorkerInfos();
//   auto iter = info_map.find(version_id);
//   if (iter == info_map.end()) {
//     thrower.ThrowError("Could not find service worker with that version_id");
//     return base::DictionaryValue();
//   }
//   return ServiceWorkerRunningInfoToDict(iter->second);
// }

// static
mate::Handle<ServiceWorkerContext> ServiceWorkerContext::Create(
    v8::Isolate* isolate,
    AtomBrowserContext* browser_context) {
  return mate::CreateHandle(isolate,
                            new ServiceWorkerContext(isolate, browser_context));
}

// static
void ServiceWorkerContext::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "ServiceWorkerContext"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate());
  // .SetMethod("getWorkerInfoFromID",
  // &ServiceWorkerContext::GetWorkerInfoFromID);
}

}  // namespace api

}  // namespace electron
