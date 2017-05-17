// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_DEVTOOLS_NETWORK_PROTOCOL_HANDLER_H_
#define BROWSER_DEVTOOLS_NETWORK_PROTOCOL_HANDLER_H_

#include "base/macros.h"
#include "base/values.h"

namespace content {
class DevToolsAgentHost;
}

namespace brightray {

class DevToolsNetworkConditions;

class DevToolsNetworkProtocolHandler {
 public:
  DevToolsNetworkProtocolHandler();
  ~DevToolsNetworkProtocolHandler();

  base::DictionaryValue* HandleCommand(
      content::DevToolsAgentHost* agent_host,
      base::DictionaryValue* command);
  void DevToolsAgentStateChanged(content::DevToolsAgentHost* agent_host,
                                 bool attached);

 private:
  std::unique_ptr<base::DictionaryValue> CanEmulateNetworkConditions(
      content::DevToolsAgentHost* agent_host,
      int command_id,
      const base::DictionaryValue* params);
  std::unique_ptr<base::DictionaryValue> EmulateNetworkConditions(
      content::DevToolsAgentHost* agent_host,
      int command_id,
      const base::DictionaryValue* params);
  void UpdateNetworkState(
      content::DevToolsAgentHost* agent_host,
      std::unique_ptr<DevToolsNetworkConditions> conditions);

  DISALLOW_COPY_AND_ASSIGN(DevToolsNetworkProtocolHandler);
};

}  // namespace brightray

#endif  // BROWSER_DEVTOOLS_NETWORK_PROTOCOL_HANDLER_H_
