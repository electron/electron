// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/lib/bluetooth_chooser.h"

#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_includes.h"

namespace gin {

template <>
struct Converter<electron::BluetoothChooser::DeviceInfo> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const electron::BluetoothChooser::DeviceInfo& val) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("deviceName", val.device_name);
    dict.Set("deviceId", val.device_id);
    return gin::ConvertToV8(isolate, dict);
  }
};

}  // namespace gin

namespace electron {

BluetoothChooser::BluetoothChooser(api::WebContents* contents,
                                   const EventHandler& event_handler)
    : api_web_contents_(contents), event_handler_(event_handler) {}

BluetoothChooser::~BluetoothChooser() {
  event_handler_.Reset();
}

// 'select-bluetooth-device'. With nobody listening the request is cancelled
// (an empty device id), which counts as handled.
bool BluetoothChooser::EmitSelectBluetoothDevice() {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> web_contents;
  if (!api_web_contents_->GetWrapper(isolate).ToLocal(&web_contents))
    return false;
  // One callback scope over the count and the emit so that pending ticks and
  // microtasks (which may run the chooser callback and delete |this|) run
  // once at the end, as they did at the end of the single emit; nothing of
  // |this| is touched after it closes. listenerCount itself is app-replaceable
  // JavaScript, so |this| and the WebContents are re-checked after it too.
  node::CallbackScope callback_scope{isolate, web_contents,
                                     node::async_context{0, 0}};
  base::WeakPtr<BluetoothChooser> weak_this = weak_ptr_factory_.GetWeakPtr();
  base::WeakPtr<api::WebContents> weak_web_contents =
      api_web_contents_->GetWeakPtr();
  int listeners = 0;
  gin::ConvertFromV8(
      isolate,
      gin_helper::CallMethod(isolate, web_contents, "listenerCount",
                             "select-bluetooth-device"),
      &listeners);
  if (!weak_this || !weak_web_contents)
    return true;
  if (listeners == 0) {
    OnDeviceChosen("");
    return true;
  }
  return api_web_contents_->Emit(
      "select-bluetooth-device", GetDeviceList(),
      base::BindOnce(&BluetoothChooser::OnDeviceChosen,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BluetoothChooser::SetAdapterPresence(AdapterPresence presence) {
  switch (presence) {
    case AdapterPresence::ABSENT:
      NOTREACHED();
    case AdapterPresence::POWERED_OFF:
      event_handler_.Run(content::BluetoothChooserEvent::CANCELLED, "");
      break;
    case AdapterPresence::UNAUTHORIZED:
      event_handler_.Run(content::BluetoothChooserEvent::DENIED_PERMISSION, "");
      break;
    case AdapterPresence::POWERED_ON:
      rescan_ = true;
      break;
  }
}

void BluetoothChooser::ShowDiscoveryState(DiscoveryState state) {
  bool idle_state = false;
  switch (state) {
    case DiscoveryState::FAILED_TO_START:
      refreshing_ = false;
      event_handler_.Run(content::BluetoothChooserEvent::CANCELLED, "");
      return;
    case DiscoveryState::IDLE:
      refreshing_ = false;
      idle_state = true;
      break;
    // The first time this state fires is due to a rescan triggering so we
    // set a flag to ignore devices - the second time this state fires
    // we are now safe to pick a device.
    case DiscoveryState::DISCOVERING:
      if (rescan_ && !refreshing_) {
        refreshing_ = true;
      } else {
        refreshing_ = false;
      }
      break;
  }

  // The handler may run the callback synchronously, which runs
  // |event_handler_| and destroys |this|.
  base::WeakPtr<BluetoothChooser> weak_this = weak_ptr_factory_.GetWeakPtr();
  bool prevent_default = EmitSelectBluetoothDevice();
  if (!weak_this)
    return;
  if (!prevent_default && idle_state) {
    if (device_id_to_name_map_.empty()) {
      event_handler_.Run(content::BluetoothChooserEvent::CANCELLED, "");
    } else {
      auto it = device_id_to_name_map_.begin();
      auto device_id = it->first;
      event_handler_.Run(content::BluetoothChooserEvent::SELECTED, device_id);
    }
  }
}

void BluetoothChooser::AddOrUpdateDevice(const std::string& device_id,
                                         bool should_update_name,
                                         const std::u16string& device_name,
                                         bool is_gatt_connected,
                                         bool is_paired,
                                         int signal_strength_level) {
  // Don't fire an event during refresh.
  if (refreshing_)
    return;

  // Emit a select-bluetooth-device handler to allow for user to listen for
  // bluetooth device found. If there's no listener in place, then select the
  // first device that matches the filters provided.
  auto [iter, changed] =
      device_id_to_name_map_.try_emplace(device_id, device_name);
  if (!changed && should_update_name) {
    iter->second = device_name;
    changed = true;
  }

  if (changed) {
    // The handler may run the callback synchronously, which runs
    // |event_handler_| and destroys |this|.
    base::WeakPtr<BluetoothChooser> weak_this = weak_ptr_factory_.GetWeakPtr();
    bool prevent_default = EmitSelectBluetoothDevice();
    if (!weak_this)
      return;

    if (!prevent_default)
      event_handler_.Run(content::BluetoothChooserEvent::SELECTED, device_id);
  }
}

void BluetoothChooser::OnDeviceChosen(const std::string& device_id) {
  if (event_handler_.is_null())
    return;

  if (device_id.empty()) {
    event_handler_.Run(content::BluetoothChooserEvent::CANCELLED, device_id);
  } else {
    event_handler_.Run(content::BluetoothChooserEvent::SELECTED, device_id);
  }
}

std::vector<electron::BluetoothChooser::DeviceInfo>
BluetoothChooser::GetDeviceList() {
  std::vector<electron::BluetoothChooser::DeviceInfo> vec;
  vec.reserve(device_id_to_name_map_.size());
  for (const auto& [device_id, device_name] : device_id_to_name_map_)
    vec.emplace_back(device_id, device_name);
  return vec;
}

}  // namespace electron
