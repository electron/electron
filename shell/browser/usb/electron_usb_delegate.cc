// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/usb/electron_usb_delegate.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/containers/cxx20_erase.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"
#include "services/device/public/mojom/usb_enumeration_options.mojom.h"
#include "shell/browser/electron_permission_manager.h"
#include "shell/browser/usb/usb_chooser_context.h"
#include "shell/browser/usb/usb_chooser_context_factory.h"
#include "shell/browser/usb/usb_chooser_controller.h"
#include "shell/browser/web_contents_permission_helper.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "base/containers/fixed_flat_set.h"
#include "chrome/common/chrome_features.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "services/device/public/mojom/usb_device.mojom.h"
#endif

namespace {

using ::content::UsbChooser;

electron::UsbChooserContext* GetChooserContext(
    content::BrowserContext* browser_context) {
  return electron::UsbChooserContextFactory::GetForBrowserContext(
      browser_context);
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
// These extensions can claim the smart card USB class and automatically gain
// permissions for devices that have an interface with this class.
constexpr auto kSmartCardPrivilegedExtensionIds =
    base::MakeFixedFlatSet<base::StringPiece>({
        // Smart Card Connector Extension and its Beta version, see
        // crbug.com/1233881.
        "khpfeaanjngmcnplbdlpegiifgpfgdco",
        "mockcojkppdndnhgonljagclgpkjbkek",
    });

bool DeviceHasInterfaceWithClass(
    const device::mojom::UsbDeviceInfo& device_info,
    uint8_t interface_class) {
  for (const auto& configuration : device_info.configurations) {
    for (const auto& interface : configuration->interfaces) {
      for (const auto& alternate : interface->alternates) {
        if (alternate->class_code == interface_class)
          return true;
      }
    }
  }
  return false;
}
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

bool IsDevicePermissionAutoGranted(
    const url::Origin& origin,
    const device::mojom::UsbDeviceInfo& device_info) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Note: The `DeviceHasInterfaceWithClass()` call is made after checking the
  // origin, since that method call is expensive.
  if (origin.scheme() == extensions::kExtensionScheme &&
      base::Contains(kSmartCardPrivilegedExtensionIds, origin.host()) &&
      DeviceHasInterfaceWithClass(device_info,
                                  device::mojom::kUsbSmartCardClass)) {
    return true;
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

  return false;
}

}  // namespace

namespace electron {

// Manages the UsbDelegate observers for a single browser context.
class ElectronUsbDelegate::ContextObservation
    : public UsbChooserContext::DeviceObserver {
 public:
  ContextObservation(ElectronUsbDelegate* parent,
                     content::BrowserContext* browser_context)
      : parent_(parent), browser_context_(browser_context) {
    auto* chooser_context = GetChooserContext(browser_context_);
    device_observation_.Observe(chooser_context);
  }
  ContextObservation(ContextObservation&) = delete;
  ContextObservation& operator=(ContextObservation&) = delete;
  ~ContextObservation() override = default;

  // UsbChooserContext::DeviceObserver:
  void OnDeviceAdded(const device::mojom::UsbDeviceInfo& device_info) override {
    for (auto& observer : observer_list_)
      observer.OnDeviceAdded(device_info);
  }

  void OnDeviceRemoved(
      const device::mojom::UsbDeviceInfo& device_info) override {
    for (auto& observer : observer_list_)
      observer.OnDeviceRemoved(device_info);
  }

  void OnDeviceManagerConnectionError() override {
    for (auto& observer : observer_list_)
      observer.OnDeviceManagerConnectionError();
  }

  void OnBrowserContextShutdown() override {
    parent_->observations_.erase(browser_context_);
    // Return since `this` is now deleted.
  }

  void AddObserver(content::UsbDelegate::Observer* observer) {
    observer_list_.AddObserver(observer);
  }

  void RemoveObserver(content::UsbDelegate::Observer* observer) {
    observer_list_.RemoveObserver(observer);
  }

 private:
  // Safe because `parent_` owns `this`.
  const raw_ptr<ElectronUsbDelegate> parent_;

  // Safe because `this` is destroyed when the context is lost.
  const raw_ptr<content::BrowserContext> browser_context_;

  base::ScopedObservation<UsbChooserContext, UsbChooserContext::DeviceObserver>
      device_observation_{this};
  base::ObserverList<content::UsbDelegate::Observer> observer_list_;
};

ElectronUsbDelegate::ElectronUsbDelegate() = default;

ElectronUsbDelegate::~ElectronUsbDelegate() = default;

void ElectronUsbDelegate::AdjustProtectedInterfaceClasses(
    content::BrowserContext* browser_context,
    const url::Origin& origin,
    content::RenderFrameHost* frame,
    std::vector<uint8_t>& classes) {
  // Isolated Apps have unrestricted access to any USB interface class.
  if (frame &&
      frame->GetWebExposedIsolationLevel() >=
          content::WebExposedIsolationLevel::kMaybeIsolatedApplication) {
    // TODO(https://crbug.com/1236706): Should the list of interface classes the
    // app expects to claim be encoded in the Web App Manifest?
    classes.clear();
    return;
  }

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Don't enforce protected interface classes for Chrome Apps since the
  // chrome.usb API has no such restriction.
  if (origin.scheme() == extensions::kExtensionScheme) {
    auto* extension_registry =
        extensions::ExtensionRegistry::Get(browser_context);
    if (extension_registry) {
      const extensions::Extension* extension =
          extension_registry->enabled_extensions().GetByID(origin.host());
      if (extension && extension->is_platform_app()) {
        classes.clear();
        return;
      }
    }
  }

  if (origin.scheme() == extensions::kExtensionScheme &&
      base::Contains(kSmartCardPrivilegedExtensionIds, origin.host())) {
    base::Erase(classes, device::mojom::kUsbSmartCardClass);
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
}

std::unique_ptr<UsbChooser> ElectronUsbDelegate::RunChooser(
    content::RenderFrameHost& frame,
    std::vector<device::mojom::UsbDeviceFilterPtr> filters,
    blink::mojom::WebUsbService::GetPermissionCallback callback) {
  UsbChooserController* controller = ControllerForFrame(&frame);
  if (controller) {
    DeleteControllerForFrame(&frame);
  }
  AddControllerForFrame(&frame, std::move(filters), std::move(callback));
  // Return a nullptr because the return value isn't used for anything. The
  // return value is simply used in Chromium to cleanup the chooser UI once the
  // usb service is destroyed.
  return nullptr;
}

bool ElectronUsbDelegate::CanRequestDevicePermission(
    content::BrowserContext* browser_context,
    const url::Origin& origin) {
  base::Value::Dict details;
  details.Set("securityOrigin", origin.GetURL().spec());
  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context->GetPermissionControllerDelegate());
  return permission_manager->CheckPermissionWithDetails(
      static_cast<blink::PermissionType>(
          WebContentsPermissionHelper::PermissionType::USB),
      nullptr, origin.GetURL(), std::move(details));
}

void ElectronUsbDelegate::RevokeDevicePermissionWebInitiated(
    content::BrowserContext* browser_context,
    const url::Origin& origin,
    const device::mojom::UsbDeviceInfo& device) {
  GetChooserContext(browser_context)
      ->RevokeDevicePermissionWebInitiated(origin, device);
}

const device::mojom::UsbDeviceInfo* ElectronUsbDelegate::GetDeviceInfo(
    content::BrowserContext* browser_context,
    const std::string& guid) {
  return GetChooserContext(browser_context)->GetDeviceInfo(guid);
}

bool ElectronUsbDelegate::HasDevicePermission(
    content::BrowserContext* browser_context,
    const url::Origin& origin,
    const device::mojom::UsbDeviceInfo& device) {
  if (IsDevicePermissionAutoGranted(origin, device))
    return true;

  return GetChooserContext(browser_context)
      ->HasDevicePermission(origin, device);
}

void ElectronUsbDelegate::GetDevices(
    content::BrowserContext* browser_context,
    blink::mojom::WebUsbService::GetDevicesCallback callback) {
  GetChooserContext(browser_context)->GetDevices(std::move(callback));
}

void ElectronUsbDelegate::GetDevice(
    content::BrowserContext* browser_context,
    const std::string& guid,
    base::span<const uint8_t> blocked_interface_classes,
    mojo::PendingReceiver<device::mojom::UsbDevice> device_receiver,
    mojo::PendingRemote<device::mojom::UsbDeviceClient> device_client) {
  GetChooserContext(browser_context)
      ->GetDevice(guid, blocked_interface_classes, std::move(device_receiver),
                  std::move(device_client));
}

void ElectronUsbDelegate::AddObserver(content::BrowserContext* browser_context,
                                      Observer* observer) {
  GetContextObserver(browser_context)->AddObserver(observer);
}

void ElectronUsbDelegate::RemoveObserver(
    content::BrowserContext* browser_context,
    Observer* observer) {
  GetContextObserver(browser_context)->RemoveObserver(observer);
}

ElectronUsbDelegate::ContextObservation*
ElectronUsbDelegate::GetContextObserver(
    content::BrowserContext* browser_context) {
  if (!base::Contains(observations_, browser_context)) {
    observations_.emplace(browser_context, std::make_unique<ContextObservation>(
                                               this, browser_context));
  }
  return observations_[browser_context].get();
}

bool ElectronUsbDelegate::IsServiceWorkerAllowedForOrigin(
    const url::Origin& origin) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // WebUSB is only available on extension service workers for now.
  if (base::FeatureList::IsEnabled(
          features::kEnableWebUsbOnExtensionServiceWorker) &&
      origin.scheme() == extensions::kExtensionScheme) {
    return true;
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
  return false;
}

UsbChooserController* ElectronUsbDelegate::ControllerForFrame(
    content::RenderFrameHost* render_frame_host) {
  auto mapping = controller_map_.find(render_frame_host);
  return mapping == controller_map_.end() ? nullptr : mapping->second.get();
}

UsbChooserController* ElectronUsbDelegate::AddControllerForFrame(
    content::RenderFrameHost* render_frame_host,
    std::vector<device::mojom::UsbDeviceFilterPtr> filters,
    blink::mojom::WebUsbService::GetPermissionCallback callback) {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto controller = std::make_unique<UsbChooserController>(
      render_frame_host, std::move(filters), std::move(callback), web_contents,
      weak_factory_.GetWeakPtr());
  controller_map_.insert(
      std::make_pair(render_frame_host, std::move(controller)));
  return ControllerForFrame(render_frame_host);
}

void ElectronUsbDelegate::DeleteControllerForFrame(
    content::RenderFrameHost* render_frame_host) {
  controller_map_.erase(render_frame_host);
}

}  // namespace electron
