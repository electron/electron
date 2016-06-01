// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/bluetooth_chooser.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "native_mate/dictionary.h"

namespace mate {

template<>
struct Converter<atom::BluetoothChooser::DeviceInfo> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate, const atom::BluetoothChooser::DeviceInfo& val) {
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

void OnDeviceChosen(
    const content::BluetoothChooser::EventHandler& handler,
    const std::string& device_id) {
  if (device_id.empty()) {
    handler.Run(content::BluetoothChooser::Event::CANCELLED, device_id);
  } else {
    handler.Run(content::BluetoothChooser::Event::SELECTED, device_id);
  }
}

}  // namespace

BluetoothChooser::BluetoothChooser(
    api::WebContents* contents,
    const EventHandler& event_handler)
    : api_web_contents_(contents),
      event_handler_(event_handler),
      num_retries_(0) {
}

BluetoothChooser::~BluetoothChooser() {
}

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
      if (device_list_.empty()) {
        auto event = ++num_retries_ > kMaxScanRetries ? Event::CANCELLED
                                                      : Event::RESCAN;
        event_handler_.Run(event, "");
      } else {
        bool prevent_default =
            api_web_contents_->Emit("select-bluetooth-device",
                                    device_list_,
                                    base::Bind(&OnDeviceChosen,
                                               event_handler_));
        if (!prevent_default) {
          auto device_id = device_list_[0].device_id;
          event_handler_.Run(Event::SELECTED, device_id);
        }
      }
      break;
    case DiscoveryState::DISCOVERING:
      break;
  }
}

void BluetoothChooser::AddDevice(const std::string& device_id,
                                 const base::string16& device_name) {
  DeviceInfo info = {device_id, device_name};
  device_list_.push_back(info);
}

void BluetoothChooser::RemoveDevice(const std::string& device_id) {
  for (auto it = device_list_.begin(); it != device_list_.end(); ++it) {
    if (it->device_id == device_id) {
      device_list_.erase(it);
      return;
    }
  }
}

}  // namespace atom
