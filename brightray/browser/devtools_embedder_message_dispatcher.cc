// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/devtools_embedder_message_dispatcher.h"

#include "base/bind.h"
#include "base/values.h"

namespace brightray {

namespace {

using DispatchCallback = DevToolsEmbedderMessageDispatcher::DispatchCallback;

bool GetValue(const base::Value& value, std::string* result) {
  return value.GetAsString(result);
}

bool GetValue(const base::Value& value, int* result) {
  return value.GetAsInteger(result);
}

bool GetValue(const base::Value& value, bool* result) {
  return value.GetAsBoolean(result);
}

bool GetValue(const base::Value& value, gfx::Rect* rect) {
  const base::DictionaryValue* dict;
  if (!value.GetAsDictionary(&dict))
    return false;
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  if (!dict->GetInteger("x", &x) ||
      !dict->GetInteger("y", &y) ||
      !dict->GetInteger("width", &width) ||
      !dict->GetInteger("height", &height))
    return false;
  rect->SetRect(x, y, width, height);
  return true;
}

template <typename T>
struct StorageTraits {
  using StorageType = T;
};

template <typename T>
struct StorageTraits<const T&> {
  using StorageType = T;
};

template <typename... Ts>
struct ParamTuple {
  bool Parse(const base::ListValue& list,
             const base::ListValue::const_iterator& it) {
    return it == list.end();
  }

  template <typename H, typename... As>
  void Apply(const H& handler, As... args) {
    handler.Run(args...);
  }
};

template <typename T, typename... Ts>
struct ParamTuple<T, Ts...> {
  bool Parse(const base::ListValue& list,
             const base::ListValue::const_iterator& it) {
    return it != list.end() && GetValue(**it, &head) &&
           tail.Parse(list, it + 1);
  }

  template <typename H, typename... As>
  void Apply(const H& handler, As... args) {
    tail.template Apply<H, As..., T>(handler, args..., head);
  }

  typename StorageTraits<T>::StorageType head;
  ParamTuple<Ts...> tail;
};

template<typename... As>
bool ParseAndHandle(const base::Callback<void(As...)>& handler,
                    const DispatchCallback& callback,
                    const base::ListValue& list) {
  ParamTuple<As...> tuple;
  if (!tuple.Parse(list, list.begin()))
    return false;
  tuple.Apply(handler);
  return true;
}

template<typename... As>
bool ParseAndHandleWithCallback(
    const base::Callback<void(const DispatchCallback&, As...)>& handler,
    const DispatchCallback& callback,
    const base::ListValue& list) {
  ParamTuple<As...> tuple;
  if (!tuple.Parse(list, list.begin()))
    return false;
  tuple.Apply(handler, callback);
  return true;
}

}  // namespace

/**
 * Dispatcher for messages sent from the frontend running in an
 * isolated renderer (chrome-devtools:// or chrome://inspect) to the embedder
 * in the browser.
 *
 * The messages are sent via InspectorFrontendHost.sendMessageToEmbedder or
 * chrome.send method accordingly.
 */
class DispatcherImpl : public DevToolsEmbedderMessageDispatcher {
 public:
  ~DispatcherImpl() override {}

  bool Dispatch(const DispatchCallback& callback,
                const std::string& method,
                const base::ListValue* params) override {
    auto it = handlers_.find(method);
    return it != handlers_.end() && it->second.Run(callback, *params);
  }

  template<typename... As>
  void RegisterHandler(const std::string& method,
                       void (Delegate::*handler)(As...),
                       Delegate* delegate) {
    handlers_[method] = base::Bind(&ParseAndHandle<As...>,
                                   base::Bind(handler,
                                              base::Unretained(delegate)));
  }

  template<typename... As>
  void RegisterHandlerWithCallback(
      const std::string& method,
      void (Delegate::*handler)(const DispatchCallback&, As...),
      Delegate* delegate) {
    handlers_[method] = base::Bind(&ParseAndHandleWithCallback<As...>,
                                   base::Bind(handler,
                                              base::Unretained(delegate)));
  }


 private:
  using Handler = base::Callback<bool(const DispatchCallback&,
                                      const base::ListValue&)>;
  using HandlerMap = std::map<std::string, Handler>;
  HandlerMap handlers_;
};

// static
DevToolsEmbedderMessageDispatcher*
DevToolsEmbedderMessageDispatcher::CreateForDevToolsFrontend(
    Delegate* delegate) {
  auto* d = new DispatcherImpl();

  d->RegisterHandler("bringToFront", &Delegate::ActivateWindow, delegate);
  d->RegisterHandler("closeWindow", &Delegate::CloseWindow, delegate);
  d->RegisterHandler("loadCompleted", &Delegate::LoadCompleted, delegate);
  d->RegisterHandler("setInspectedPageBounds",
                     &Delegate::SetInspectedPageBounds, delegate);
  d->RegisterHandler("inspectElementCompleted",
                     &Delegate::InspectElementCompleted, delegate);
  d->RegisterHandler("inspectedURLChanged",
                     &Delegate::InspectedURLChanged, delegate);
  d->RegisterHandlerWithCallback("setIsDocked",
                                 &Delegate::SetIsDocked, delegate);
  d->RegisterHandler("openInNewTab", &Delegate::OpenInNewTab, delegate);
  d->RegisterHandler("save", &Delegate::SaveToFile, delegate);
  d->RegisterHandler("append", &Delegate::AppendToFile, delegate);
  d->RegisterHandler("requestFileSystems",
                     &Delegate::RequestFileSystems, delegate);
  d->RegisterHandler("addFileSystem", &Delegate::AddFileSystem, delegate);
  d->RegisterHandler("removeFileSystem", &Delegate::RemoveFileSystem, delegate);
  d->RegisterHandler("upgradeDraggedFileSystemPermissions",
                     &Delegate::UpgradeDraggedFileSystemPermissions, delegate);
  d->RegisterHandler("indexPath", &Delegate::IndexPath, delegate);
  d->RegisterHandlerWithCallback("loadNetworkResource",
                                 &Delegate::LoadNetworkResource, delegate);
  d->RegisterHandler("stopIndexing", &Delegate::StopIndexing, delegate);
  d->RegisterHandler("searchInPath", &Delegate::SearchInPath, delegate);
  d->RegisterHandler("setWhitelistedShortcuts",
                     &Delegate::SetWhitelistedShortcuts, delegate);
  d->RegisterHandler("zoomIn", &Delegate::ZoomIn, delegate);
  d->RegisterHandler("zoomOut", &Delegate::ZoomOut, delegate);
  d->RegisterHandler("resetZoom", &Delegate::ResetZoom, delegate);
  d->RegisterHandler("setDevicesUpdatesEnabled",
                     &Delegate::SetDevicesUpdatesEnabled, delegate);
  d->RegisterHandler("dispatchProtocolMessage",
                     &Delegate::DispatchProtocolMessageFromDevToolsFrontend,
                     delegate);
  d->RegisterHandler("recordActionUMA", &Delegate::RecordActionUMA, delegate);
  d->RegisterHandlerWithCallback("sendJsonRequest",
                                 &Delegate::SendJsonRequest, delegate);
  d->RegisterHandlerWithCallback("getPreferences",
                                 &Delegate::GetPreferences, delegate);
  d->RegisterHandler("setPreference", &Delegate::SetPreference, delegate);
  d->RegisterHandler("removePreference", &Delegate::RemovePreference, delegate);
  d->RegisterHandler("clearPreferences", &Delegate::ClearPreferences, delegate);
  return d;
}

}  // namespace brightray
