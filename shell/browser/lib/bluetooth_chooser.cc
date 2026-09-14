// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/lib/bluetooth_chooser.h"

#include "base/task/sequenced_task_runner.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "gin/data_object_builder.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
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

BluetoothChooser::BluetoothChooser(content::RenderFrameHost* render_frame_host,
                                   const EventHandler& event_handler)
    : render_frame_host_id_(render_frame_host->GetGlobalId()),
      event_handler_(event_handler) {}

void BluetoothChooser::RunEventHandler(content::BluetoothChooserEvent event,
                                       const std::string& device_id) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<BluetoothChooser> self,
             content::BluetoothChooserEvent event, std::string device_id) {
            if (self && !self->event_handler_.is_null())
              self->event_handler_.Run(event, device_id);
          },
          weak_ptr_factory_.GetWeakPtr(), event, device_id));
}

bool BluetoothChooser::EmitSelectEvent(const DeviceInfo* added_or_updated) {
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_frame_host_id_);
  bool prevent_default = false;

  // Session-level events, shaped like select-hid-device / hid-device-added.
  gin::WeakCell<api::Session>* session =
      rfh ? api::Session::FromBrowserContext(rfh->GetBrowserContext())
          : nullptr;
  if (session && session->Get()) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    base::WeakPtr<BluetoothChooser> weak_this = weak_ptr_factory_.GetWeakPtr();
    if (!session_select_emitted_) {
      session_select_emitted_ = true;
      v8::Local<v8::Object> details = gin::DataObjectBuilder(isolate)
                                          .Set("deviceList", GetDeviceList())
                                          .Set("frame", rfh)
                                          .Build();
      prevent_default |=
          session->Get()->Emit("select-bluetooth-device", details,
                               base::BindOnce(&BluetoothChooser::OnDeviceChosen,
                                              weak_ptr_factory_.GetWeakPtr()));
    } else if (added_or_updated) {
      v8::Local<v8::Object> details = gin::DataObjectBuilder(isolate)
                                          .Set("device", *added_or_updated)
                                          .Set("frame", rfh)
                                          .Build();
      session->Get()->Emit("bluetooth-device-added", details);
    }
    if (!weak_this)
      return prevent_default;
  }

  // WebContents-level event (deprecated): re-emitted with the full list every
  // time it changes. The JS wrapper is looked up each time; it can be torn
  // down before the document that owns this chooser (e.g. a <webview> guest).
  api::WebContents* api_web_contents =
      rfh ? api::WebContents::From(
                content::WebContents::FromRenderFrameHost(rfh))
          : nullptr;
  if (api_web_contents) {
    prevent_default |=
        api_web_contents->Emit("select-bluetooth-device", GetDeviceList(),
                               base::BindOnce(&BluetoothChooser::OnDeviceChosen,
                                              weak_ptr_factory_.GetWeakPtr()),
                               rfh);
  }
  return prevent_default;
}

BluetoothChooser::~BluetoothChooser() {
  event_handler_.Reset();
}

void BluetoothChooser::SetAdapterPresence(AdapterPresence presence) {
  switch (presence) {
    case AdapterPresence::ABSENT:
      NOTREACHED();
    case AdapterPresence::POWERED_OFF:
      RunEventHandler(content::BluetoothChooserEvent::CANCELLED, "");
      break;
    case AdapterPresence::UNAUTHORIZED:
      RunEventHandler(content::BluetoothChooserEvent::DENIED_PERMISSION, "");
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
      RunEventHandler(content::BluetoothChooserEvent::CANCELLED, "");
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
  const bool handled = EmitSelectEvent(nullptr);
  if (!weak_this)
    return;
  handled_ |= handled;
  // Discovery finished and no listener took responsibility for answering:
  // cancel the request rather than picking a device on the app's behalf.
  if (idle_state && !handled_)
    RunEventHandler(content::BluetoothChooserEvent::CANCELLED, "");
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

  // Emit select-bluetooth-device / bluetooth-device-added when a device is
  // first seen or its name actually changes, so the app can pick a device as
  // soon as it appears. Nothing is selected unless the app calls the callback.
  auto [iter, inserted] =
      device_id_to_name_map_.try_emplace(device_id, device_name);
  bool changed = inserted;
  if (!inserted && should_update_name && iter->second != device_name) {
    iter->second = device_name;
    changed = true;
  }
  if (!changed)
    return;

  // The handler may run the callback synchronously, which destroys |this|.
  base::WeakPtr<BluetoothChooser> weak_this = weak_ptr_factory_.GetWeakPtr();
  const DeviceInfo info{device_id, iter->second};
  const bool handled = EmitSelectEvent(&info);
  if (!weak_this)
    return;
  handled_ |= handled;
}

void BluetoothChooser::OnDeviceChosen(const std::string& device_id) {
  if (event_handler_.is_null())
    return;

  RunEventHandler(device_id.empty() ? content::BluetoothChooserEvent::CANCELLED
                                    : content::BluetoothChooserEvent::SELECTED,
                  device_id);
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
