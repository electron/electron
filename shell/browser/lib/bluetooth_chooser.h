// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_
#define ELECTRON_SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/bluetooth_chooser.h"
#include "content/public/browser/global_routing_id.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace electron {

class BluetoothChooser : public content::BluetoothChooser {
 public:
  struct DeviceInfo {
    std::string device_id;
    std::u16string device_name;
  };

  BluetoothChooser(content::RenderFrameHost* render_frame_host,
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
  // Emits select-bluetooth-device on the session (once, then
  // bluetooth-device-added for later devices) and on the WebContents
  // (deprecated, every time); returns true if a listener took responsibility
  // for answering (event.preventDefault()). May delete |this|.
  bool EmitSelectEvent(const DeviceInfo* added_or_updated);

  // Runs |event_handler_|. Always posted: content destroys this chooser from
  // inside the handler, and may be part-way through its own work (e.g.
  // StartDeviceDiscovery) when a listener answers synchronously.
  void RunEventHandler(content::BluetoothChooserEvent event,
                       const std::string& device_id);

  absl::flat_hash_map<std::string, std::u16string> device_id_to_name_map_;
  content::GlobalRenderFrameHostId render_frame_host_id_;
  EventHandler event_handler_;
  bool refreshing_ = false;
  bool rescan_ = false;
  // Whether any select-bluetooth-device listener has called preventDefault()
  // for this chooser, i.e. the app will answer through the callback.
  bool handled_ = false;
  // Whether the session-level select-bluetooth-device event has been emitted
  // for this chooser.
  bool session_select_emitted_ = false;

  base::WeakPtrFactory<BluetoothChooser> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_
