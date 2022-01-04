// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_
#define ELECTRON_SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_

#include <map>
#include <string>
#include <vector>

#include "content/public/browser/bluetooth_chooser.h"
#include "shell/browser/api/electron_api_web_contents.h"

namespace electron {

class BluetoothChooser : public content::BluetoothChooser {
 public:
  struct DeviceInfo {
    std::string device_id;
    std::u16string device_name;
  };

  explicit BluetoothChooser(api::WebContents* contents,
                            const EventHandler& handler);
  ~BluetoothChooser() override;

  // disable copy
  BluetoothChooser(const BluetoothChooser&) = delete;
  BluetoothChooser& operator=(const BluetoothChooser&) = delete;

  // content::BluetoothChooser:
  void SetAdapterPresence(AdapterPresence presence) override;
  void ShowDiscoveryState(DiscoveryState state) override;
  void AddOrUpdateDevice(const std::string& device_id,
                         bool should_update_name,
                         const std::u16string& device_name,
                         bool is_gatt_connected,
                         bool is_paired,
                         int signal_strength_level) override;
  std::vector<DeviceInfo> GetDeviceList();

 private:
  std::map<std::string, std::u16string> device_map_;
  api::WebContents* api_web_contents_;
  EventHandler event_handler_;
  int num_retries_ = 0;
  bool refreshing_ = false;
  bool rescan_ = false;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_
