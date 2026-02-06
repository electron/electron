// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/hid/hid_chooser_context.h"

#include <string_view>
#include <utility>

#include "base/command_line.h"
#include "base/containers/map_util.h"
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
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "base/containers/fixed_flat_set.h"
#include "extensions/common/constants.h"
#endif  // BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)

namespace {

// Adapted from third_party/blink/renderer/modules/hid/hid_device.cc.
enum HidUnitSystem {
  // none: No unit system
  kUnitSystemNone = 0x00,
  // si-linear: Centimeter, Gram, Seconds, Kelvin, Ampere, Candela
  kUnitSystemSILinear = 0x01,
  // si-rotation: Radians, Gram, Seconds, Kelvin, Ampere, Candela
  kUnitSystemSIRotation = 0x02,
  // english-linear: Inch, Slug, Seconds, Fahrenheit, Ampere, Candela
  kUnitSystemEnglishLinear = 0x03,
  // english-linear: Degrees, Slug, Seconds, Fahrenheit, Ampere, Candela
  kUnitSystemEnglishRotation = 0x04,
  // vendor-defined unit system
  kUnitSystemVendorDefined = 0x0f,
};

// Adapted from third_party/blink/renderer/modules/hid/hid_device.cc.
int ConvertHidUsageAndPageToInt(const device::mojom::HidUsageAndPage& usage) {
  return static_cast<int>((usage.usage_page) << 16 | usage.usage);
}

// Adapted from third_party/blink/renderer/modules/hid/hid_device.cc.
int8_t UnitFactorExponentToInt(uint8_t unit_factor_exponent) {
  DCHECK_LE(unit_factor_exponent, 0x0f);
  // Values from 0x08 to 0x0f encode negative exponents.
  if (unit_factor_exponent > 0x08)
    return static_cast<int8_t>(unit_factor_exponent) - 16;
  return unit_factor_exponent;
}

// Adapted from third_party/blink/renderer/modules/hid/hid_device.cc.
std::string UnitSystemToString(uint8_t unit) {
  DCHECK_LE(unit, 0x0f);
  switch (unit) {
    case kUnitSystemNone:
      return "none";
    case kUnitSystemSILinear:
      return "si-linear";
    case kUnitSystemSIRotation:
      return "si-rotation";
    case kUnitSystemEnglishLinear:
      return "english-linear";
    case kUnitSystemEnglishRotation:
      return "english-rotation";
    case kUnitSystemVendorDefined:
      return "vendor-defined";
    default:
      break;
  }
  // Values other than those defined in HidUnitSystem are reserved by the spec.
  return "reserved";
}

// Adapted from third_party/blink/renderer/modules/hid/hid_device.cc.
base::DictValue HidReportItemToValue(const device::mojom::HidReportItem& item) {
  base::DictValue dict;

  dict.Set("hasNull", item.has_null_position);
  dict.Set("hasPreferredState", !item.no_preferred_state);
  dict.Set("isAbsolute", !item.is_relative);
  dict.Set("isArray", !item.is_variable);
  dict.Set("isBufferedBytes", item.is_buffered_bytes);
  dict.Set("isConstant", item.is_constant);
  dict.Set("isLinear", !item.is_non_linear);
  dict.Set("isRange", item.is_range);
  dict.Set("isVolatile", item.is_volatile);
  dict.Set("logicalMinimum", item.logical_minimum);
  dict.Set("logicalMaximum", item.logical_maximum);
  dict.Set("physicalMinimum", item.physical_minimum);
  dict.Set("physicalMaximum", item.physical_maximum);
  dict.Set("reportCount", static_cast<int>(item.report_count));
  dict.Set("reportSize", static_cast<int>(item.report_size));

  dict.Set("unitExponent", UnitFactorExponentToInt(item.unit_exponent & 0x0f));
  dict.Set("unitFactorCurrentExponent",
           UnitFactorExponentToInt((item.unit >> 20) & 0x0f));
  dict.Set("unitFactorLengthExponent",
           UnitFactorExponentToInt((item.unit >> 4) & 0x0f));
  dict.Set("unitFactorLuminousIntensityExponent",
           UnitFactorExponentToInt((item.unit >> 24) & 0x0f));
  dict.Set("unitFactorMassExponent",
           UnitFactorExponentToInt((item.unit >> 8) & 0x0f));
  dict.Set("unitFactorTemperatureExponent",
           UnitFactorExponentToInt((item.unit >> 16) & 0x0f));
  dict.Set("unitFactorTimeExponent",
           UnitFactorExponentToInt((item.unit >> 12) & 0x0f));
  dict.Set("unitSystem", UnitSystemToString(item.unit & 0x0f));

  if (item.is_range) {
    dict.Set("usageMinimum", ConvertHidUsageAndPageToInt(*item.usage_minimum));
    dict.Set("usageMaximum", ConvertHidUsageAndPageToInt(*item.usage_maximum));
  } else {
    base::ListValue usages_list;
    for (const auto& usage : item.usages) {
      usages_list.Append(ConvertHidUsageAndPageToInt(*usage));
    }
    dict.Set("usages", std::move(usages_list));
  }

  dict.Set("wrap", item.wrap);

  return dict;
}

// Adapted from third_party/blink/renderer/modules/hid/hid_device.cc.
base::DictValue HidReportDescriptionToValue(
    const device::mojom::HidReportDescription& report) {
  base::DictValue dict;
  dict.Set("reportId", static_cast<int>(report.report_id));

  base::ListValue items_list;
  for (const auto& item : report.items) {
    items_list.Append(base::Value(HidReportItemToValue(*item)));
  }
  dict.Set("items", std::move(items_list));

  return dict;
}

// Adapted from third_party/blink/renderer/modules/hid/hid_device.cc.
base::DictValue HidCollectionInfoToValue(
    const device::mojom::HidCollectionInfo& collection) {
  base::DictValue dict;

  // Usage information
  dict.Set("usage", collection.usage->usage);
  dict.Set("usagePage", collection.usage->usage_page);

  // Collection type
  dict.Set("collectionType", static_cast<int>(collection.collection_type));

  // Input reports
  base::ListValue input_reports_list;
  for (const auto& report : collection.input_reports) {
    input_reports_list.Append(
        base::Value(HidReportDescriptionToValue(*report)));
  }
  dict.Set("inputReports", std::move(input_reports_list));

  // Output reports
  base::ListValue output_reports_list;
  for (const auto& report : collection.output_reports) {
    output_reports_list.Append(
        base::Value(HidReportDescriptionToValue(*report)));
  }
  dict.Set("outputReports", std::move(output_reports_list));

  // Feature reports
  base::ListValue feature_reports_list;
  for (const auto& report : collection.feature_reports) {
    feature_reports_list.Append(
        base::Value(HidReportDescriptionToValue(*report)));
  }
  dict.Set("featureReports", std::move(feature_reports_list));

  // Child collections (recursive)
  base::ListValue children_list;
  for (const auto& child : collection.children) {
    children_list.Append(base::Value(HidCollectionInfoToValue(*child)));
  }
  dict.Set("children", std::move(children_list));

  return dict;
}
}  // namespace

namespace electron {

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
        absl::StrFormat("%04X:%04X", device.vendor_id, device.product_id));
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
  base::DictValue value;
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

  // Convert collections array
  base::ListValue collections_list;
  for (const auto& collection : device.collections) {
    collections_list.Append(base::Value(HidCollectionInfoToValue(*collection)));
  }
  value.Set("collections", std::move(collections_list));

  return base::Value(std::move(value));
}

void HidChooserContext::GrantDevicePermission(
    const url::Origin& origin,
    const device::mojom::HidDeviceInfo& device) {
  DCHECK(devices_.contains(device.guid));
  if (CanStorePersistentEntry(device)) {
    auto* permission_manager = static_cast<ElectronPermissionManager*>(
        browser_context_->GetPermissionControllerDelegate());

    permission_manager->GrantDevicePermission(blink::PermissionType::HID,
                                              origin, DeviceInfoToValue(device),
                                              browser_context_);
  } else {
    ephemeral_devices_[origin].insert(device.guid);
  }
}

void HidChooserContext::RevokeDevicePermission(
    const url::Origin& origin,
    const device::mojom::HidDeviceInfo& device) {
  DCHECK(devices_.contains(device.guid));
  if (CanStorePersistentEntry(device)) {
    RevokePersistentDevicePermission(origin, device);
  } else {
    RevokeEphemeralDevicePermission(origin, device);
  }
  gin::WeakCell<api::Session>* session =
      api::Session::FromBrowserContext(browser_context_);
  if (session && session->Get()) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    auto details = gin_helper::Dictionary::CreateEmpty(isolate);
    details.Set("device", device.Clone());
    details.Set("origin", origin.Serialize());
    session->Get()->Emit("hid-device-revoked", details);
  }
}

void HidChooserContext::RevokePersistentDevicePermission(
    const url::Origin& origin,
    const device::mojom::HidDeviceInfo& device) {
  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context_->GetPermissionControllerDelegate());
  permission_manager->RevokeDevicePermission(blink::PermissionType::HID, origin,
                                             DeviceInfoToValue(device),
                                             browser_context_);
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
  if (it != ephemeral_devices_.end() && it->second.contains(device.guid)) {
    return true;
  }

  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context_->GetPermissionControllerDelegate());
  return permission_manager->CheckDevicePermission(
      blink::PermissionType::HID, origin, DeviceInfoToValue(device),
      browser_context_);
}

bool HidChooserContext::IsFidoAllowedForOrigin(const url::Origin& origin) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  static constexpr auto kPrivilegedExtensionIds =
      base::MakeFixedFlatSet<std::string_view>({
          "ckcendljdlmgnhghiaomidhiiclmapok",  // gnubbyd-v3 dev
          "lfboplenmmjcmpbkeemecobbadnmpfhi",  // gnubbyd-v3 prod
      });

  if (origin.scheme() == extensions::kExtensionScheme &&
      kPrivilegedExtensionIds.contains(origin.host())) {
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
  if (!devices_.contains(device->guid))
    devices_.insert({device->guid, device->Clone()});

  // Notify all observers.
  device_observer_list_.Notify(&DeviceObserver::OnDeviceAdded, *device);
}

void HidChooserContext::DeviceRemoved(device::mojom::HidDeviceInfoPtr device) {
  DCHECK(device);

  // Update the device list.
  const size_t n_erased = devices_.erase(device->guid);
  DCHECK_EQ(n_erased, 1U);

  // Notify all device observers.
  device_observer_list_.Notify(&DeviceObserver::OnDeviceRemoved, *device);

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

  // Update the device list.
  auto& mapped = devices_[device->guid];
  DCHECK(!mapped.is_null());
  mapped = device->Clone();

  // Notify all observers.
  device_observer_list_.Notify(&DeviceObserver::OnDeviceChanged, *device);
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
    devices_.try_emplace(device->guid, std::move(device));

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
  device_observer_list_.Notify(&DeviceObserver::OnHidManagerConnectionError);
}

}  // namespace electron
