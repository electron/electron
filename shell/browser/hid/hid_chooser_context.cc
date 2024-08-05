// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/hid/hid_chooser_context.h"

#include <string_view>
#include <utility>

#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/containers/map_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/grit/generated_resources.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/device_service.h"
#include "services/device/public/cpp/hid/hid_blocklist.h"
#include "services/device/public/cpp/hid/hid_switches.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/electron_permission_manager.h"
#include "shell/browser/web_contents_permission_helper.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/hid_device_info_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "base/containers/fixed_flat_set.h"
#include "extensions/common/constants.h"
#endif  // BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)

namespace electron {

const char kHidDeviceNameKey[] = "name";
const char kHidGuidKey[] = "guid";

HidChooserContext::HidChooserContext(ElectronBrowserContext* context)
    : browser_context_(context) {}

HidChooserContext::~HidChooserContext() {
  // Notify observers that the chooser context is about to be destroyed.
  // Observers must remove themselves from the observer lists.
  for (auto& observer : device_observer_list_) {
    observer.OnHidChooserContextShutdown();
    DCHECK(!device_observer_list_.HasObserver(&observer));
  }
}

// static
std::u16string HidChooserContext::DisplayNameFromDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  if (device.product_name.empty()) {
    auto device_id_string = base::ASCIIToUTF16(
        base::StringPrintf("%04X:%04X", device.vendor_id, device.product_id));
    return l10n_util::GetStringFUTF16(IDS_HID_CHOOSER_ITEM_WITHOUT_NAME,
                                      device_id_string);
  }
  return base::UTF8ToUTF16(device.product_name);
}

// static
bool HidChooserContext::CanStorePersistentEntry(
    const device::mojom::HidDeviceInfo& device) {
  return !device.serial_number.empty() && !device.product_name.empty();
}

// static
base::Value HidChooserContext::DeviceInfoToValue(
    const device::mojom::HidDeviceInfo& device) {
  base::Value::Dict value;
  value.Set(
      kHidDeviceNameKey,
      base::UTF16ToUTF8(HidChooserContext::DisplayNameFromDeviceInfo(device)));
  value.Set(kDeviceVendorIdKey, device.vendor_id);
  value.Set(kDeviceProductIdKey, device.product_id);
  if (HidChooserContext::CanStorePersistentEntry(device)) {
    // Use the USB serial number as a persistent identifier. If it is
    // unavailable, only ephemeral permissions may be granted.
    value.Set(kDeviceSerialNumberKey, device.serial_number);
  } else {
    // The GUID is a temporary ID created on connection that remains valid until
    // the device is disconnected. Ephemeral permissions are keyed by this ID
    // and must be granted again each time the device is connected.
    value.Set(kHidGuidKey, device.guid);
  }
  return base::Value(std::move(value));
}

void HidChooserContext::GrantDevicePermission(
    const url::Origin& origin,
    const device::mojom::HidDeviceInfo& device) {
  DCHECK(base::Contains(devices_, device.guid));
  if (CanStorePersistentEntry(device)) {
    auto* permission_manager = static_cast<ElectronPermissionManager*>(
        browser_context_->GetPermissionControllerDelegate());

    permission_manager->GrantDevicePermission(
        static_cast<blink::PermissionType>(
            WebContentsPermissionHelper::PermissionType::HID),
        origin, DeviceInfoToValue(device), browser_context_);
  } else {
    ephemeral_devices_[origin].insert(device.guid);
  }
}

void HidChooserContext::RevokeDevicePermission(
    const url::Origin& origin,
    const device::mojom::HidDeviceInfo& device) {
  DCHECK(base::Contains(devices_, device.guid));
  if (CanStorePersistentEntry(device)) {
    RevokePersistentDevicePermission(origin, device);
  } else {
    RevokeEphemeralDevicePermission(origin, device);
  }
  api::Session* session = api::Session::FromBrowserContext(browser_context_);
  if (session) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    auto details = gin_helper::Dictionary::CreateEmpty(isolate);
    details.Set("device", device.Clone());
    details.Set("origin", origin.Serialize());
    session->Emit("hid-device-revoked", details);
  }
}

void HidChooserContext::RevokePersistentDevicePermission(
    const url::Origin& origin,
    const device::mojom::HidDeviceInfo& device) {
  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context_->GetPermissionControllerDelegate());
  permission_manager->RevokeDevicePermission(
      static_cast<blink::PermissionType>(
          WebContentsPermissionHelper::PermissionType::HID),
      origin, DeviceInfoToValue(device), browser_context_);
  RevokeEphemeralDevicePermission(origin, device);
}

void HidChooserContext::RevokeEphemeralDevicePermission(
    const url::Origin& origin,
    const device::mojom::HidDeviceInfo& device) {
  auto it = ephemeral_devices_.find(origin);
  if (it == ephemeral_devices_.end())
    return;

  std::set<std::string>& device_guids = it->second;
  std::erase_if(device_guids, [&](const auto& guid) {
    auto* device_ptr = base::FindPtrOrNull(devices_, guid);
    return device_ptr &&
           device_ptr->physical_device_id == device.physical_device_id;
  });

  if (device_guids.empty())
    ephemeral_devices_.erase(it);
}

bool HidChooserContext::HasDevicePermission(
    const url::Origin& origin,
    const device::mojom::HidDeviceInfo& device) {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableHidBlocklist) &&
      device.is_excluded_by_blocklist)
    return false;

  auto it = ephemeral_devices_.find(origin);
  if (it != ephemeral_devices_.end() &&
      base::Contains(it->second, device.guid)) {
    return true;
  }

  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context_->GetPermissionControllerDelegate());
  return permission_manager->CheckDevicePermission(
      static_cast<blink::PermissionType>(
          WebContentsPermissionHelper::PermissionType::HID),
      origin, DeviceInfoToValue(device), browser_context_);
}

bool HidChooserContext::IsFidoAllowedForOrigin(const url::Origin& origin) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  static constexpr auto kPrivilegedExtensionIds =
      base::MakeFixedFlatSet<std::string_view>({
          "ckcendljdlmgnhghiaomidhiiclmapok",  // gnubbyd-v3 dev
          "lfboplenmmjcmpbkeemecobbadnmpfhi",  // gnubbyd-v3 prod
      });

  if (origin.scheme() == extensions::kExtensionScheme &&
      base::Contains(kPrivilegedExtensionIds, origin.host())) {
    return true;
  }
#endif  // BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)

  // This differs from upstream - we want to allow users greater
  // ability to communicate with FIDO devices in Electron.
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableHidBlocklist);
}

void HidChooserContext::AddDeviceObserver(DeviceObserver* observer) {
  EnsureHidManagerConnection();
  device_observer_list_.AddObserver(observer);
}

void HidChooserContext::RemoveDeviceObserver(DeviceObserver* observer) {
  device_observer_list_.RemoveObserver(observer);
}

void HidChooserContext::GetDevices(
    device::mojom::HidManager::GetDevicesCallback callback) {
  if (!is_initialized_) {
    EnsureHidManagerConnection();
    pending_get_devices_requests_.push(std::move(callback));
    return;
  }

  std::vector<device::mojom::HidDeviceInfoPtr> device_list;
  device_list.reserve(devices_.size());
  for (const auto& pair : devices_)
    device_list.push_back(pair.second->Clone());
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(device_list)));
}

const device::mojom::HidDeviceInfo* HidChooserContext::GetDeviceInfo(
    const std::string& guid) {
  DCHECK(is_initialized_);
  auto it = devices_.find(guid);
  return it == devices_.end() ? nullptr : it->second.get();
}

device::mojom::HidManager* HidChooserContext::GetHidManager() {
  EnsureHidManagerConnection();
  return hid_manager_.get();
}

base::WeakPtr<HidChooserContext> HidChooserContext::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void HidChooserContext::DeviceAdded(device::mojom::HidDeviceInfoPtr device) {
  DCHECK(device);

  // Update the device list.
  if (!base::Contains(devices_, device->guid))
    devices_.insert({device->guid, device->Clone()});

  // Notify all observers.
  for (auto& observer : device_observer_list_)
    observer.OnDeviceAdded(*device);
}

void HidChooserContext::DeviceRemoved(device::mojom::HidDeviceInfoPtr device) {
  DCHECK(device);
  DCHECK(base::Contains(devices_, device->guid));

  // Update the device list.
  devices_.erase(device->guid);

  // Notify all device observers.
  for (auto& observer : device_observer_list_)
    observer.OnDeviceRemoved(*device);

  // Next we'll notify observers for revoked permissions. If the device does not
  // support persistent permissions then device permissions are revoked on
  // disconnect.
  if (CanStorePersistentEntry(*device))
    return;

  for (auto& [origin, guids] : ephemeral_devices_)
    guids.erase(device->guid);
}

void HidChooserContext::DeviceChanged(device::mojom::HidDeviceInfoPtr device) {
  DCHECK(device);
  DCHECK(base::Contains(devices_, device->guid));

  // Update the device list.
  devices_[device->guid] = device->Clone();

  // Notify all observers.
  for (auto& observer : device_observer_list_)
    observer.OnDeviceChanged(*device);
}

void HidChooserContext::EnsureHidManagerConnection() {
  if (hid_manager_)
    return;

  mojo::PendingRemote<device::mojom::HidManager> manager;
  content::GetDeviceService().BindHidManager(
      manager.InitWithNewPipeAndPassReceiver());
  SetUpHidManagerConnection(std::move(manager));
}

void HidChooserContext::SetUpHidManagerConnection(
    mojo::PendingRemote<device::mojom::HidManager> manager) {
  hid_manager_.Bind(std::move(manager));
  hid_manager_.set_disconnect_handler(base::BindOnce(
      &HidChooserContext::OnHidManagerConnectionError, base::Unretained(this)));

  hid_manager_->GetDevicesAndSetClient(
      client_receiver_.BindNewEndpointAndPassRemote(),
      base::BindOnce(&HidChooserContext::InitDeviceList,
                     weak_factory_.GetWeakPtr()));
}

void HidChooserContext::InitDeviceList(
    std::vector<device::mojom::HidDeviceInfoPtr> devices) {
  for (auto& device : devices)
    devices_.insert({device->guid, std::move(device)});

  is_initialized_ = true;

  while (!pending_get_devices_requests_.empty()) {
    std::vector<device::mojom::HidDeviceInfoPtr> device_list;
    device_list.reserve(devices.size());
    for (const auto& entry : devices_)
      device_list.push_back(entry.second->Clone());
    std::move(pending_get_devices_requests_.front())
        .Run(std::move(device_list));
    pending_get_devices_requests_.pop();
  }
}

void HidChooserContext::OnHidManagerConnectionError() {
  hid_manager_.reset();
  client_receiver_.reset();
  devices_.clear();
  ephemeral_devices_.clear();

  // Notify all device observers.
  for (auto& observer : device_observer_list_)
    observer.OnHidManagerConnectionError();
}

}  // namespace electron
