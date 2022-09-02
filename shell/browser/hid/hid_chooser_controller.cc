// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/hid/hid_chooser_controller.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/ranges/algorithm.h"
#include "base/stl_util.h"
#include "gin/data_object_builder.h"
#include "services/device/public/cpp/hid/hid_blocklist.h"
#include "services/device/public/cpp/hid/hid_switches.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/hid/hid_chooser_context.h"
#include "shell/browser/hid/hid_chooser_context_factory.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/hid_device_info_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/process_util.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

bool FilterMatch(const blink::mojom::HidDeviceFilterPtr& filter,
                 const device::mojom::HidDeviceInfo& device) {
  if (filter->device_ids) {
    if (filter->device_ids->is_vendor()) {
      if (filter->device_ids->get_vendor() != device.vendor_id)
        return false;
    } else if (filter->device_ids->is_vendor_and_product()) {
      const auto& vendor_and_product =
          filter->device_ids->get_vendor_and_product();
      if (vendor_and_product->vendor != device.vendor_id)
        return false;
      if (vendor_and_product->product != device.product_id)
        return false;
    }
  }

  if (filter->usage) {
    if (filter->usage->is_page()) {
      const uint16_t usage_page = filter->usage->get_page();
      auto find_it =
          std::find_if(device.collections.begin(), device.collections.end(),
                       [=](const device::mojom::HidCollectionInfoPtr& c) {
                         return usage_page == c->usage->usage_page;
                       });
      if (find_it == device.collections.end())
        return false;
    } else if (filter->usage->is_usage_and_page()) {
      const auto& usage_and_page = filter->usage->get_usage_and_page();
      auto find_it = std::find_if(
          device.collections.begin(), device.collections.end(),
          [&usage_and_page](const device::mojom::HidCollectionInfoPtr& c) {
            return usage_and_page->usage_page == c->usage->usage_page &&
                   usage_and_page->usage == c->usage->usage;
          });
      if (find_it == device.collections.end())
        return false;
    }
  }
  return true;
}

}  // namespace

namespace electron {

HidChooserController::HidChooserController(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::HidDeviceFilterPtr> filters,
    std::vector<blink::mojom::HidDeviceFilterPtr> exclusion_filters,
    content::HidChooser::Callback callback,
    content::WebContents* web_contents,
    base::WeakPtr<ElectronHidDelegate> hid_delegate)
    : WebContentsObserver(web_contents),
      filters_(std::move(filters)),
      exclusion_filters_(std::move(exclusion_filters)),
      callback_(std::move(callback)),
      origin_(content::WebContents::FromRenderFrameHost(render_frame_host)
                  ->GetPrimaryMainFrame()
                  ->GetLastCommittedOrigin()),
      frame_tree_node_id_(render_frame_host->GetFrameTreeNodeId()),
      hid_delegate_(hid_delegate),
      render_frame_host_id_(render_frame_host->GetGlobalId()) {
  chooser_context_ = HidChooserContextFactory::GetForBrowserContext(
                         web_contents->GetBrowserContext())
                         ->AsWeakPtr();
  DCHECK(chooser_context_);

  chooser_context_->GetHidManager()->GetDevices(base::BindOnce(
      &HidChooserController::OnGotDevices, weak_factory_.GetWeakPtr()));
}

HidChooserController::~HidChooserController() {
  if (callback_)
    std::move(callback_).Run(std::vector<device::mojom::HidDeviceInfoPtr>());
}

// static
std::string HidChooserController::PhysicalDeviceIdFromDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  // A single physical device may expose multiple HID interfaces, each
  // represented by a HidDeviceInfo object. When a device exposes multiple
  // HID interfaces, the HidDeviceInfo objects will share a common
  // |physical_device_id|. Group these devices so that a single chooser item
  // is shown for each physical device. If a device's physical device ID is
  // empty, use its GUID instead.
  return device.physical_device_id.empty() ? device.guid
                                           : device.physical_device_id;
}

api::Session* HidChooserController::GetSession() {
  if (!web_contents()) {
    return nullptr;
  }
  return api::Session::FromBrowserContext(web_contents()->GetBrowserContext());
}

void HidChooserController::OnDeviceAdded(
    const device::mojom::HidDeviceInfo& device) {
  if (!DisplayDevice(device))
    return;
  if (AddDeviceInfo(device)) {
    api::Session* session = GetSession();
    if (session) {
      auto* rfh = content::RenderFrameHost::FromID(render_frame_host_id_);
      v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
      v8::HandleScope scope(isolate);
      v8::Local<v8::Object> details = gin::DataObjectBuilder(isolate)
                                          .Set("device", device.Clone())
                                          .Set("frame", rfh)
                                          .Build();
      session->Emit("hid-device-added", details);
    }
  }

  return;
}

void HidChooserController::OnDeviceRemoved(
    const device::mojom::HidDeviceInfo& device) {
  auto id = PhysicalDeviceIdFromDeviceInfo(device);
  auto items_it = std::find(items_.begin(), items_.end(), id);
  if (items_it == items_.end())
    return;
  api::Session* session = GetSession();
  if (session) {
    auto* rfh = content::RenderFrameHost::FromID(render_frame_host_id_);
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Object> details = gin::DataObjectBuilder(isolate)
                                        .Set("device", device.Clone())
                                        .Set("frame", rfh)
                                        .Build();
    session->Emit("hid-device-removed", details);
  }
  RemoveDeviceInfo(device);
}

void HidChooserController::OnDeviceChanged(
    const device::mojom::HidDeviceInfo& device) {
  bool has_chooser_item =
      base::Contains(items_, PhysicalDeviceIdFromDeviceInfo(device));
  if (!DisplayDevice(device)) {
    if (has_chooser_item)
      OnDeviceRemoved(device);
    return;
  }

  if (!has_chooser_item) {
    OnDeviceAdded(device);
    return;
  }

  // Update the item to replace the old device info with |device|.
  UpdateDeviceInfo(device);
}

void HidChooserController::OnDeviceChosen(gin::Arguments* args) {
  std::string device_id;
  if (!args->GetNext(&device_id) || device_id.empty()) {
    RunCallback({});
  } else {
    auto find_it = device_map_.find(device_id);
    if (find_it != device_map_.end()) {
      auto& device_infos = find_it->second;
      std::vector<device::mojom::HidDeviceInfoPtr> devices;
      devices.reserve(device_infos.size());
      for (auto& device : device_infos) {
        chooser_context_->GrantDevicePermission(origin_, *device);
        devices.push_back(device->Clone());
      }
      RunCallback(std::move(devices));
    } else {
      v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
      node::Environment* env = node::Environment::GetCurrent(isolate);
      EmitWarning(env, "The device id " + device_id + " was not found.",
                  "UnknownHIDDeviceId");
      RunCallback({});
    }
  }
}

void HidChooserController::OnHidManagerConnectionError() {
  observation_.Reset();
}

void HidChooserController::OnHidChooserContextShutdown() {
  observation_.Reset();
}

void HidChooserController::OnGotDevices(
    std::vector<device::mojom::HidDeviceInfoPtr> devices) {
  std::vector<device::mojom::HidDeviceInfoPtr> devicesToDisplay;
  devicesToDisplay.reserve(devices.size());

  for (auto& device : devices) {
    if (DisplayDevice(*device)) {
      if (AddDeviceInfo(*device)) {
        devicesToDisplay.push_back(device->Clone());
      }
    }
  }

  // Listen to HidChooserContext for OnDeviceAdded/Removed events after the
  // enumeration.
  if (chooser_context_)
    observation_.Observe(chooser_context_.get());
  bool prevent_default = false;
  api::Session* session = GetSession();
  if (session) {
    auto* rfh = content::RenderFrameHost::FromID(render_frame_host_id_);
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Object> details = gin::DataObjectBuilder(isolate)
                                        .Set("deviceList", devicesToDisplay)
                                        .Set("frame", rfh)
                                        .Build();
    prevent_default =
        session->Emit("select-hid-device", details,
                      base::AdaptCallbackForRepeating(
                          base::BindOnce(&HidChooserController::OnDeviceChosen,
                                         weak_factory_.GetWeakPtr())));
  }
  if (!prevent_default) {
    RunCallback({});
  }
}

bool HidChooserController::DisplayDevice(
    const device::mojom::HidDeviceInfo& device) const {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableHidBlocklist)) {
    // Do not pass the device to the chooser if it is excluded by the blocklist.
    if (device.is_excluded_by_blocklist)
      return false;

    // Do not pass the device to the chooser if it has a top-level collection
    // with the FIDO usage page.
    //
    // Note: The HID blocklist also blocks top-level collections with the FIDO
    // usage page, but will not block the device if it has other (non-FIDO)
    // collections. The check below will exclude the device from the chooser
    // if it has any top-level FIDO collection.
    auto find_it =
        std::find_if(device.collections.begin(), device.collections.end(),
                     [](const device::mojom::HidCollectionInfoPtr& c) {
                       return c->usage->usage_page == device::mojom::kPageFido;
                     });
    if (find_it != device.collections.end())
      return false;
  }

  return FilterMatchesAny(device) && !IsExcluded(device);
}

bool HidChooserController::FilterMatchesAny(
    const device::mojom::HidDeviceInfo& device) const {
  if (filters_.empty())
    return true;

  for (const auto& filter : filters_) {
    if (FilterMatch(filter, device))
      return true;
  }

  return false;
}

bool HidChooserController::IsExcluded(
    const device::mojom::HidDeviceInfo& device) const {
  for (const auto& exclusion_filter : exclusion_filters_) {
    if (FilterMatch(exclusion_filter, device))
      return true;
  }

  return false;
}

bool HidChooserController::AddDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  auto id = PhysicalDeviceIdFromDeviceInfo(device);
  auto find_it = device_map_.find(id);
  if (find_it != device_map_.end()) {
    find_it->second.push_back(device.Clone());
    return false;
  }
  // A new device was connected. Append it to the end of the chooser list.
  device_map_[id].push_back(device.Clone());
  items_.push_back(id);
  return true;
}

bool HidChooserController::RemoveDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  auto id = PhysicalDeviceIdFromDeviceInfo(device);
  auto find_it = device_map_.find(id);
  DCHECK(find_it != device_map_.end());
  auto& device_infos = find_it->second;
  base::EraseIf(device_infos,
                [&device](const device::mojom::HidDeviceInfoPtr& d) {
                  return d->guid == device.guid;
                });
  if (!device_infos.empty())
    return false;
  // A device was disconnected. Remove it from the chooser list.
  device_map_.erase(find_it);
  base::Erase(items_, id);
  return true;
}

void HidChooserController::UpdateDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  auto id = PhysicalDeviceIdFromDeviceInfo(device);
  auto physical_device_it = device_map_.find(id);
  DCHECK(physical_device_it != device_map_.end());
  auto& device_infos = physical_device_it->second;
  auto device_it = base::ranges::find_if(
      device_infos, [&device](const device::mojom::HidDeviceInfoPtr& d) {
        return d->guid == device.guid;
      });
  DCHECK(device_it != device_infos.end());
  *device_it = device.Clone();
}

void HidChooserController::RunCallback(
    std::vector<device::mojom::HidDeviceInfoPtr> devices) {
  if (callback_) {
    std::move(callback_).Run(std::move(devices));
  }
}

void HidChooserController::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (hid_delegate_) {
    hid_delegate_->DeleteControllerForFrame(render_frame_host);
  }
}

}  // namespace electron
