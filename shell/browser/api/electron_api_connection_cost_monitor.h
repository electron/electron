// Copyright (c) 2026 Microsoft, GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CONNECTION_COST_MONITOR_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CONNECTION_COST_MONITOR_H_

#include "gin/wrappable.h"
#include "net/base/network_change_notifier.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/self_keep_alive.h"

namespace electron::api {

class ConnectionCostMonitor final
    : public gin::Wrappable<ConnectionCostMonitor>,
      public gin_helper::EventEmitterMixin<ConnectionCostMonitor>,
      private net::NetworkChangeNotifier::ConnectionCostObserver {
 public:
  static ConnectionCostMonitor* Create(v8::Isolate* isolate);

  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetClassName() const { return "ConnectionCostMonitor"; }

  ConnectionCostMonitor();
  ~ConnectionCostMonitor() override;

  ConnectionCostMonitor(const ConnectionCostMonitor&) = delete;
  ConnectionCostMonitor& operator=(const ConnectionCostMonitor&) = delete;

 private:
  // net::NetworkChangeNotifier::ConnectionCostObserver
  void OnConnectionCostChanged(
      net::NetworkChangeNotifier::ConnectionCost cost) override;

  gin_helper::SelfKeepAlive<ConnectionCostMonitor> keep_alive_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CONNECTION_COST_MONITOR_H_
