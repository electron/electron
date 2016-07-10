// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/net/devtools_network_protocol_handler.h"

#include "browser/browser_context.h"
#include "browser/net/devtools_network_conditions.h"
#include "browser/net/devtools_network_controller.h"

#include "base/strings/stringprintf.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"


namespace brightray {

namespace {

namespace params {

const char kDownloadThroughput[] = "downloadThroughput";
const char kLatency[] = "latency";
const char kOffline[] = "offline";
const char kUploadThroughput[] = "uploadThroughput";
const char kResult[] = "result";
const char kErrorCode[] = "code";
const char kErrorMessage[] = "message";

}  //  namespace params

const char kEmulateNetworkConditions[] = "Network.emulateNetworkConditions";
const char kCanEmulateNetworkConditions[] = "Network.canEmulateNetworkConditions";
const char kId[] = "id";
const char kMethod[] = "method";
const char kParams[] = "params";
const char kError[] = "error";
// JSON RPC 2.0 spec: http://www.jsonrpc.org/specification#error_object
const int kErrorInvalidParams = -32602;


bool ParseCommand(const base::DictionaryValue* command,
                  int* id,
                  std::string* method,
                  const base::DictionaryValue** params) {
  if (!command)
    return false;

  if (!command->GetInteger(kId, id) || *id < 0)
    return false;

  if (!command->GetString(kMethod, method))
    return false;

  if (!command->GetDictionary(kParams, params))
    *params = nullptr;

  return true;
}

std::unique_ptr<base::DictionaryValue>
CreateSuccessResponse(int id, std::unique_ptr<base::DictionaryValue> result) {
  std::unique_ptr<base::DictionaryValue> response(new base::DictionaryValue);
  response->SetInteger(kId, id);
  response->Set(params::kResult, result.release());
  return response;
}

std::unique_ptr<base::DictionaryValue>
CreateFailureResponse(int id, const std::string& param) {
  std::unique_ptr<base::DictionaryValue> response(new base::DictionaryValue);
  auto error_object = new base::DictionaryValue;
  response->Set(kError, error_object);
  error_object->SetInteger(params::kErrorCode, kErrorInvalidParams);
  error_object->SetString(params::kErrorMessage,
      base::StringPrintf("Missing or Invalid '%s' parameter", param.c_str()));
  return response;
}

}  // namespace

DevToolsNetworkProtocolHandler::DevToolsNetworkProtocolHandler() {
}

DevToolsNetworkProtocolHandler::~DevToolsNetworkProtocolHandler() {
}

base::DictionaryValue* DevToolsNetworkProtocolHandler::HandleCommand(
    content::DevToolsAgentHost* agent_host,
    base::DictionaryValue* command) {
  int id = 0;
  std::string method;
  const base::DictionaryValue* params = nullptr;

  if (!ParseCommand(command, &id, &method, &params))
    return nullptr;

  if (method == kEmulateNetworkConditions)
    return EmulateNetworkConditions(agent_host, id, params).release();

  if (method == kCanEmulateNetworkConditions)
    return CanEmulateNetworkConditions(agent_host, id, params).release();

  return nullptr;
}

void DevToolsNetworkProtocolHandler::DevToolsAgentStateChanged(
    content::DevToolsAgentHost* agent_host,
    bool attached) {
  std::unique_ptr<DevToolsNetworkConditions> conditions;
  if (attached)
    conditions.reset(new DevToolsNetworkConditions(false));
  UpdateNetworkState(agent_host, std::move(conditions));
}

std::unique_ptr<base::DictionaryValue>
DevToolsNetworkProtocolHandler::CanEmulateNetworkConditions(
    content::DevToolsAgentHost* agent_host,
    int id,
    const base::DictionaryValue* params) {
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue);
  result->SetBoolean(params::kResult, true);
  return CreateSuccessResponse(id, std::move(result));
}

std::unique_ptr<base::DictionaryValue>
DevToolsNetworkProtocolHandler::EmulateNetworkConditions(
    content::DevToolsAgentHost* agent_host,
    int id,
    const base::DictionaryValue* params) {
  bool offline = false;
  if (!params || !params->GetBoolean(params::kOffline, &offline))
    return CreateFailureResponse(id, params::kOffline);

  double latency = 0.0;
  if (!params->GetDouble(params::kLatency, &latency))
    return CreateFailureResponse(id, params::kLatency);
  if (latency < 0.0)
    latency = 0.0;

  double download_throughput = 0.0;
  if (!params->GetDouble(params::kDownloadThroughput, &download_throughput))
    return CreateFailureResponse(id, params::kDownloadThroughput);
  if (download_throughput < 0.0)
    download_throughput = 0.0;

  double upload_throughput = 0.0;
  if (!params->GetDouble(params::kUploadThroughput, &upload_throughput))
    return CreateFailureResponse(id, params::kUploadThroughput);
  if (upload_throughput < 0.0)
    upload_throughput = 0.0;

  std::unique_ptr<DevToolsNetworkConditions> conditions(
      new DevToolsNetworkConditions(offline,
                                    latency,
                                    download_throughput,
                                    upload_throughput));
  UpdateNetworkState(agent_host, std::move(conditions));
  return std::unique_ptr<base::DictionaryValue>();
}

void DevToolsNetworkProtocolHandler::UpdateNetworkState(
    content::DevToolsAgentHost* agent_host,
    std::unique_ptr<DevToolsNetworkConditions> conditions) {
  auto browser_context =
      static_cast<brightray::BrowserContext*>(agent_host->GetBrowserContext());
  browser_context->network_controller_handle()->SetNetworkState(
      agent_host->GetId(), std::move(conditions));
}

}  // namespace brightray
