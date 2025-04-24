// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_
#define ELECTRON_SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/bluetooth_chooser.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

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

  void OnDeviceChosen(const std::string& device_id);
  std::vector<DeviceInfo> GetDeviceList();

 private:
  absl::flat_hash_map<std::string, std::u16string> device_id_to_name_map_;
  raw_ptr<api::WebContents> api_web_contents_;
  EventHandler event_handler_;
  bool refreshing_ = false;
  bool rescan_ = false;

  base::WeakPtrFactory<BluetoothChooser> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_
