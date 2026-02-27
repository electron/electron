// Copyright (c) 2026 Microsoft, GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_connection_cost_monitor.h"

#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "v8/include/cppgc/allocation.h"

namespace electron::api {

const gin::WrapperInfo ConnectionCostMonitor::kWrapperInfo = {
    {gin::kEmbedderNativeGin},
    gin::kElectronConnectionCostMonitor};

ConnectionCostMonitor::ConnectionCostMonitor() {
  net::NetworkChangeNotifier::AddConnectionCostObserver(this);
}

ConnectionCostMonitor::~ConnectionCostMonitor() {
  net::NetworkChangeNotifier::RemoveConnectionCostObserver(this);
}

void ConnectionCostMonitor::OnConnectionCostChanged(
    net::NetworkChangeNotifier::ConnectionCost cost) {
  bool is_metered =
      cost == net::NetworkChangeNotifier::CONNECTION_COST_METERED;
  EmitWithoutEvent("connection-cost-changed", is_metered);
}

// static
ConnectionCostMonitor* ConnectionCostMonitor::Create(v8::Isolate* isolate) {
  return cppgc::MakeGarbageCollected<ConnectionCostMonitor>(
      isolate->GetCppHeap()->GetAllocationHandle());
}

gin::ObjectTemplateBuilder ConnectionCostMonitor::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<
      ConnectionCostMonitor>::GetObjectTemplateBuilder(isolate);
}

const gin::WrapperInfo* ConnectionCostMonitor::wrapper_info() const {
  return &kWrapperInfo;
}

const char* ConnectionCostMonitor::GetHumanReadableName() const {
  return "Electron / ConnectionCostMonitor";
}

}  // namespace electron::api

namespace {

using electron::api::ConnectionCostMonitor;

bool IsConnectionMetered() {
  return net::NetworkChangeNotifier::GetConnectionCost() ==
         net::NetworkChangeNotifier::CONNECTION_COST_METERED;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("createConnectionCostMonitor",
                 base::BindRepeating(&ConnectionCostMonitor::Create));
  dict.SetMethod("isConnectionMetered",
                 base::BindRepeating(&IsConnectionMetered));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_connection_cost_monitor,
                                  Initialize)
