// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/lib/bluetooth_chooser.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "native_mate/dictionary.h"

namespace mate {

template <>
struct Converter<atom::BluetoothChooser::DeviceInfo> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const atom::BluetoothChooser::DeviceInfo& val) {
    mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
    dict.Set("deviceName", val.device_name);
    dict.Set("deviceId", val.device_id);
    return mate::ConvertToV8(isolate, dict);
  }
};

}  // namespace mate

namespace atom {

namespace {

const int kMaxScanRetries = 5;

void OnDeviceChosen(const content::BluetoothChooser::EventHandler& handler,
                    const std::string& device_id) {
  if (device_id.empty()) {
    handler.Run(content::BluetoothChooser::Event::CANCELLED, device_id);
  } else {
    handler.Run(content::BluetoothChooser::Event::SELECTED, device_id);
  }
}

}  // namespace

BluetoothChooser::BluetoothChooser(api::WebContents* contents,
                                   const EventHandler& event_handler)
    : api_web_contents_(contents), event_handler_(event_handler) {}

BluetoothChooser::~BluetoothChooser() {}

void BluetoothChooser::SetAdapterPresence(AdapterPresence presence) {
  switch (presence) {
    case AdapterPresence::ABSENT:
    case AdapterPresence::POWERED_OFF:
      event_handler_.Run(Event::CANCELLED, "");
      break;
    case AdapterPresence::POWERED_ON:
      break;
  }
}

void BluetoothChooser::ShowDiscoveryState(DiscoveryState state) {
  switch (state) {
    case DiscoveryState::FAILED_TO_START:
      event_handler_.Run(Event::CANCELLED, "");
      break;
    case DiscoveryState::IDLE:
      if (device_map_.empty()) {
        auto event =
            ++num_retries_ > kMaxScanRetries ? Event::CANCELLED : Event::RESCAN;
        event_handler_.Run(event, "");
      } else {
        bool prevent_default = api_web_contents_->Emit(
            "select-bluetooth-device", GetDeviceList(),
            base::Bind(&OnDeviceChosen, event_handler_));
        if (!prevent_default) {
          auto it = device_map_.begin();
          auto device_id = it->first;
          event_handler_.Run(Event::SELECTED, device_id);
        }
      }
      break;
    case DiscoveryState::DISCOVERING:
      break;
  }
}

void BluetoothChooser::AddOrUpdateDevice(const std::string& device_id,
                                         bool should_update_name,
                                         const base::string16& device_name,
                                         bool is_gatt_connected,
                                         bool is_paired,
                                         int signal_strength_level) {
  bool changed = false;
  auto entry = device_map_.find(device_id);
  if (entry == device_map_.end()) {
    device_map_[device_id] = device_name;
    changed = true;
  } else if (should_update_name) {
    entry->second = device_name;
    changed = true;
  }

  if (changed) {
    // Emit a select-bluetooth-device handler to allow for user to listen for
    // bluetooth device found.
    bool prevent_default =
        api_web_contents_->Emit("select-bluetooth-device", GetDeviceList(),
                                base::Bind(&OnDeviceChosen, event_handler_));

    // If emit not implimented select first device that matches the filters
    //  provided.
    if (!prevent_default) {
      event_handler_.Run(Event::SELECTED, device_id);
    }
  }
}

std::vector<atom::BluetoothChooser::DeviceInfo>
BluetoothChooser::GetDeviceList() {
  std::vector<atom::BluetoothChooser::DeviceInfo> vec;
  for (const auto& it : device_map_) {
    DeviceInfo info = {it.first, it.second};
    vec.push_back(info);
  }

  return vec;
}

}  // namespace atom
