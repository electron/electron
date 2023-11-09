// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/lib/bluetooth_chooser.h"

#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"

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

namespace {

void OnDeviceChosen(const content::BluetoothChooser::EventHandler& handler,
                    const std::string& device_id) {
  if (device_id.empty()) {
    handler.Run(content::BluetoothChooserEvent::CANCELLED, device_id);
  } else {
    handler.Run(content::BluetoothChooserEvent::SELECTED, device_id);
  }
}

}  // namespace

BluetoothChooser::BluetoothChooser(api::WebContents* contents,
                                   const EventHandler& event_handler)
    : api_web_contents_(contents), event_handler_(event_handler) {}

BluetoothChooser::~BluetoothChooser() {
  event_handler_.Reset();
}

void BluetoothChooser::SetAdapterPresence(AdapterPresence presence) {
  switch (presence) {
    case AdapterPresence::ABSENT:
    case AdapterPresence::POWERED_OFF:
    // Chrome currently directs the user to system preferences
    // to grant bluetooth permission for this case, should we
    // do something similar ?
    // https://chromium-review.googlesource.com/c/chromium/src/+/2617129
    case AdapterPresence::UNAUTHORIZED:
      event_handler_.Run(content::BluetoothChooserEvent::CANCELLED, "");
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
    case DiscoveryState::DISCOVERING:
      // The first time this state fires is due to a rescan triggering so set a
      // flag to ignore devices
      if (rescan_ && !refreshing_) {
        refreshing_ = true;
      } else {
        // The second time this state fires we are now safe to pick a device
        refreshing_ = false;
      }
      break;
  }
  bool prevent_default =
      api_web_contents_->Emit("select-bluetooth-device", GetDeviceList(),
                              base::BindOnce(&OnDeviceChosen, event_handler_));
  if (!prevent_default && idle_state) {
    if (device_map_.empty()) {
      event_handler_.Run(content::BluetoothChooserEvent::CANCELLED, "");
    } else {
      auto it = device_map_.begin();
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
  if (refreshing_) {
    // If the list of bluetooth devices is currently being generated don't fire
    // an event
    return;
  }

  auto [iter, changed] = device_map_.try_emplace(device_id, device_name);
  if (!changed && should_update_name) {
    iter->second = device_name;
    changed = true;
  }

  if (changed) {
    // Emit a select-bluetooth-device handler to allow for user to listen for
    // bluetooth device found.
    bool prevent_default = api_web_contents_->Emit(
        "select-bluetooth-device", GetDeviceList(),
        base::BindOnce(&OnDeviceChosen, event_handler_));

    // If emit not implemented select first device that matches the filters
    //  provided.
    if (!prevent_default) {
      event_handler_.Run(content::BluetoothChooserEvent::SELECTED, device_id);
    }
  }
}

std::vector<electron::BluetoothChooser::DeviceInfo>
BluetoothChooser::GetDeviceList() {
  std::vector<electron::BluetoothChooser::DeviceInfo> vec;
  vec.reserve(device_map_.size());
  for (const auto& [device_id, device_name] : device_map_)
    vec.emplace_back(device_id, device_name);
  return vec;
}

}  // namespace electron
