// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_permission_manager.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/containers/to_vector.h"
#include "base/values.h"
#include "content/browser/permissions/permission_util.h"  // nogncheck
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "gin/data_object_builder.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/web_contents_permission_helper.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/usb_protected_classes_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

namespace electron {

namespace {

bool WebContentsDestroyed(content::RenderFrameHost* rfh) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents)
    return true;
  return web_contents->IsBeingDestroyed();
}

void PermissionRequestResponseCallbackWrapper(
    ElectronPermissionManager::StatusCallback callback,
    const std::vector<content::PermissionResult>& vector) {
  std::move(callback).Run(vector[0]);
}

}  // namespace

class ElectronPermissionManager::PendingRequest {
 public:
  PendingRequest(content::RenderFrameHost* render_frame_host,
                 std::vector<blink::mojom::PermissionDescriptorPtr> permissions,
                 StatusesCallback callback)
      : render_frame_host_id_(render_frame_host->GetGlobalId()),
        callback_(std::move(callback)),
        permissions_(std::move(permissions)),
        results_(permissions_.size(),
                 content::PermissionResult(
                     blink::mojom::PermissionStatus::DENIED,
                     content::PermissionStatusSource::UNSPECIFIED)),
        remaining_results_(permissions_.size()) {}

  void SetPermissionStatus(int permission_id,
                           blink::mojom::PermissionStatus status) {
    DCHECK(!IsComplete());

    if (status == blink::mojom::PermissionStatus::GRANTED) {
      const auto permission = blink::PermissionDescriptorToPermissionType(
          permissions_[permission_id]);
      if (permission == blink::PermissionType::MIDI_SYSEX) {
        // TODO: remove `GetUnsafeValue()` once `GrantSendMidiSysExMessage`
        // accepts `ChildProcessId`
        content::ChildProcessSecurityPolicy::GetInstance()
            ->GrantSendMidiSysExMessage(
                render_frame_host_id_.child_id.GetUnsafeValue());
      } else if (permission == blink::PermissionType::GEOLOCATION) {
        ElectronBrowserMainParts::Get()
            ->GetGeolocationControl()
            ->UserDidOptIntoLocationServices();
      }
    }

    results_[permission_id] = content::PermissionResult(
        status, content::PermissionStatusSource::UNSPECIFIED);
    --remaining_results_;
  }

  content::RenderFrameHost* GetRenderFrameHost() {
    return content::RenderFrameHost::FromID(render_frame_host_id_);
  }

  [[nodiscard]] bool IsComplete() const { return remaining_results_ == 0; }

  void RunCallback() {
    if (!callback_.is_null()) {
      std::move(callback_).Run(results_);
    }
  }

 private:
  content::GlobalRenderFrameHostId render_frame_host_id_;
  StatusesCallback callback_;
  std::vector<blink::mojom::PermissionDescriptorPtr> permissions_;
  std::vector<content::PermissionResult> results_;
  size_t remaining_results_;
};

ElectronPermissionManager::ElectronPermissionManager() = default;

ElectronPermissionManager::~ElectronPermissionManager() = default;

void ElectronPermissionManager::SetPermissionRequestHandler(
    const RequestHandler& handler) {
  if (handler.is_null() && !pending_requests_.IsEmpty()) {
    for (PendingRequestsMap::iterator iter(&pending_requests_); !iter.IsAtEnd();
         iter.Advance()) {
      auto* request = iter.GetCurrentValue();
      if (!WebContentsDestroyed(request->GetRenderFrameHost()))
        request->RunCallback();
    }
    pending_requests_.Clear();
  }
  request_handler_ = handler;
}

void ElectronPermissionManager::SetPermissionCheckHandler(
    const CheckHandler& handler) {
  check_handler_ = handler;
}

void ElectronPermissionManager::SetDevicePermissionHandler(
    const DeviceCheckHandler& handler) {
  device_permission_handler_ = handler;
}

void ElectronPermissionManager::SetProtectedUSBHandler(
    const ProtectedUSBHandler& handler) {
  protected_usb_handler_ = handler;
}

void ElectronPermissionManager::SetBluetoothPairingHandler(
    const BluetoothPairingHandler& handler) {
  bluetooth_pairing_handler_ = handler;
}

// static
bool ElectronPermissionManager::IsGeolocationDisabledViaCommandLine() {
// Remove platform check once flag is extended to other platforms
#if BUILDFLAG(IS_MAC)
  auto* command_line = base::CommandLine::ForCurrentProcess();
  return command_line->HasSwitch("disable-geolocation");
#else
  return false;
#endif
}

bool ElectronPermissionManager::HasPermissionRequestHandler() const {
  return !request_handler_.is_null();
}

bool ElectronPermissionManager::HasPermissionCheckHandler() const {
  return !check_handler_.is_null();
}

void ElectronPermissionManager::RequestPermissionWithDetails(
    blink::mojom::PermissionDescriptorPtr permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    base::DictValue details,
    StatusCallback response_callback) {
  if (render_frame_host->IsNestedWithinFencedFrame()) {
    std::move(response_callback)
        .Run(content::PermissionResult(
            blink::mojom::PermissionStatus::DENIED,
            content::PermissionStatusSource::UNSPECIFIED));
    return;
  }

  RequestPermissionsWithDetails(
      render_frame_host,
      content::PermissionRequestDescription(std::move(permission), user_gesture,
                                            requesting_origin),
      std::move(details),
      base::BindOnce(PermissionRequestResponseCallbackWrapper,
                     std::move(response_callback)));
}

void ElectronPermissionManager::RequestPermissions(
    content::RenderFrameHost* render_frame_host,
    const content::PermissionRequestDescription& request_description,
    StatusesCallback callback) {
  if (render_frame_host->IsNestedWithinFencedFrame()) {
    std::move(callback).Run(std::vector<content::PermissionResult>(
        request_description.permissions.size(),
        content::PermissionResult(
            blink::mojom::PermissionStatus::DENIED,
            content::PermissionStatusSource::UNSPECIFIED)));
    return;
  }

  RequestPermissionsWithDetails(render_frame_host, request_description, {},
                                std::move(callback));
}

void ElectronPermissionManager::RequestPermissionsWithDetails(
    content::RenderFrameHost* render_frame_host,
    const content::PermissionRequestDescription& request_description,
    base::DictValue details,
    StatusesCallback response_callback) {
  if (request_description.permissions.empty()) {
    std::move(response_callback).Run({});
    return;
  }

  auto permissions = base::ToVector(request_description.permissions,
                                    [](const auto& permission_descriptor) {
                                      return permission_descriptor.Clone();
                                    });

  if (request_handler_.is_null()) {
    std::vector<content::PermissionResult> results;
    for (const auto& permission : permissions) {
      const auto permission_type =
          blink::PermissionDescriptorToPermissionType(permission);
      if (permission_type == blink::PermissionType::MIDI_SYSEX) {
        content::ChildProcessSecurityPolicy::GetInstance()
            ->GrantSendMidiSysExMessage(
                render_frame_host->GetProcess()->GetDeprecatedID());
      } else if (permission_type == blink::PermissionType::GEOLOCATION) {
        if (IsGeolocationDisabledViaCommandLine()) {
          results.emplace_back(blink::mojom::PermissionStatus::DENIED,
                               content::PermissionStatusSource::UNSPECIFIED);
          continue;
        } else {
          ElectronBrowserMainParts::Get()
              ->GetGeolocationControl()
              ->UserDidOptIntoLocationServices();
        }
      }
      results.emplace_back(blink::mojom::PermissionStatus::GRANTED,
                           content::PermissionStatusSource::UNSPECIFIED);
    }
    std::move(response_callback).Run(results);
    return;
  }

  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  int request_id = pending_requests_.Add(std::make_unique<PendingRequest>(
      render_frame_host, std::move(permissions), std::move(response_callback)));

  details.Set("requestingUrl", render_frame_host->GetLastCommittedURL().spec());
  details.Set("isMainFrame", render_frame_host->GetParent() == nullptr);
  base::Value dict_value(std::move(details));

  for (size_t i = 0; i < request_description.permissions.size(); ++i) {
    const auto permission = blink::PermissionDescriptorToPermissionType(
        request_description.permissions[i]);
    const auto callback =
        base::BindRepeating(&ElectronPermissionManager::OnPermissionResponse,
                            base::Unretained(this), request_id, i);
    request_handler_.Run(web_contents, permission, callback, dict_value);
  }
}

void ElectronPermissionManager::OnPermissionResponse(
    int request_id,
    int permission_id,
    content::PermissionResult result) {
  auto* pending_request = pending_requests_.Lookup(request_id);
  if (!pending_request)
    return;

  pending_request->SetPermissionStatus(permission_id, result.status);
  if (pending_request->IsComplete()) {
    pending_request->RunCallback();
    pending_requests_.Remove(request_id);
  }
}

void ElectronPermissionManager::ResetPermission(
    blink::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {}

void ElectronPermissionManager::RequestPermissionsFromCurrentDocument(
    content::RenderFrameHost* render_frame_host,
    const content::PermissionRequestDescription& request_description,
    base::OnceCallback<void(const std::vector<content::PermissionResult>&)>
        callback) {
  if (render_frame_host->IsNestedWithinFencedFrame()) {
    std::move(callback).Run(std::vector<content::PermissionResult>(
        request_description.permissions.size(),
        content::PermissionResult(
            blink::mojom::PermissionStatus::DENIED,
            content::PermissionStatusSource::UNSPECIFIED)));
    return;
  }

  RequestPermissionsWithDetails(render_frame_host, request_description, {},
                                std::move(callback));
}

blink::mojom::PermissionStatus ElectronPermissionManager::GetPermissionStatus(
    const blink::mojom::PermissionDescriptorPtr& permission_descriptor,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  const auto permission =
      blink::PermissionDescriptorToPermissionType(permission_descriptor);
  base::DictValue details;
  details.Set("embeddingOrigin", embedding_origin.spec());
  bool granted = CheckPermissionWithDetails(permission, {}, requesting_origin,
                                            std::move(details));
  return granted ? blink::mojom::PermissionStatus::GRANTED
                 : blink::mojom::PermissionStatus::DENIED;
}

content::PermissionResult
ElectronPermissionManager::GetPermissionResultForOriginWithoutContext(
    const blink::mojom::PermissionDescriptorPtr& permission_descriptor,
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin) {
  blink::mojom::PermissionStatus status =
      GetPermissionStatus(permission_descriptor, requesting_origin.GetURL(),
                          embedding_origin.GetURL());
  return content::PermissionResult(status);
}

void ElectronPermissionManager::CheckBluetoothDevicePair(
    gin_helper::Dictionary details,
    PairCallback pair_callback) const {
  if (bluetooth_pairing_handler_.is_null()) {
    base::DictValue response;
    response.Set("confirmed", false);
    std::move(pair_callback).Run(std::move(response));
  } else {
    bluetooth_pairing_handler_.Run(details, std::move(pair_callback));
  }
}

bool ElectronPermissionManager::CheckPermissionWithDetails(
    blink::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    base::DictValue details) const {
  if (permission == blink::PermissionType::GEOLOCATION &&
      IsGeolocationDisabledViaCommandLine())
    return false;

  if (check_handler_.is_null()) {
    if (permission == blink::PermissionType::DEPRECATED_SYNC_CLIPBOARD_READ) {
      return false;
    } else {
      return true;
    }
  }

  auto* web_contents =
      render_frame_host
          ? content::WebContents::FromRenderFrameHost(render_frame_host)
          : nullptr;
  if (render_frame_host) {
    details.Set("requestingUrl",
                render_frame_host->GetLastCommittedURL().spec());
  }
  details.Set("isMainFrame",
              render_frame_host && render_frame_host->GetParent() == nullptr);
  switch (permission) {
    case blink::PermissionType::AUDIO_CAPTURE:
      details.Set("mediaType", "audio");
      break;
    case blink::PermissionType::VIDEO_CAPTURE:
      details.Set("mediaType", "video");
      break;
    default:
      break;
  }
  return check_handler_.Run(web_contents, permission, requesting_origin,
                            base::Value(std::move(details)));
}

bool ElectronPermissionManager::CheckDevicePermission(
    blink::PermissionType permission,
    const url::Origin& origin,
    const base::Value& device,
    ElectronBrowserContext* browser_context) const {
  if (permission == blink::PermissionType::GEOLOCATION &&
      IsGeolocationDisabledViaCommandLine())
    return false;

  if (device_permission_handler_.is_null())
    return browser_context->CheckDevicePermission(origin, device, permission);

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> details = gin::DataObjectBuilder(isolate)
                                      .Set("deviceType", permission)
                                      .Set("origin", origin.Serialize())
                                      .Set("device", device.Clone())
                                      .Build();
  return device_permission_handler_.Run(details);
}

void ElectronPermissionManager::GrantDevicePermission(
    blink::PermissionType permission,
    const url::Origin& origin,
    const base::Value& device,
    ElectronBrowserContext* browser_context) const {
  if (device_permission_handler_.is_null()) {
    browser_context->GrantDevicePermission(origin, device, permission);
  }
}

void ElectronPermissionManager::RevokeDevicePermission(
    blink::PermissionType permission,
    const url::Origin& origin,
    const base::Value& device,
    ElectronBrowserContext* browser_context) const {
  browser_context->RevokeDevicePermission(origin, device, permission);
}

ElectronPermissionManager::USBProtectedClasses
ElectronPermissionManager::CheckProtectedUSBClasses(
    const USBProtectedClasses& classes) const {
  if (protected_usb_handler_.is_null())
    return classes;

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> details =
      gin::DataObjectBuilder(isolate).Set("protectedClasses", classes).Build();
  return protected_usb_handler_.Run(details);
}

content::PermissionResult
ElectronPermissionManager::GetPermissionResultForCurrentDocument(
    const blink::mojom::PermissionDescriptorPtr& permission_descriptor,
    content::RenderFrameHost* render_frame_host,
    bool should_include_device_status) {
  if (render_frame_host->IsNestedWithinFencedFrame())
    return content::PermissionResult(blink::mojom::PermissionStatus::DENIED);

  const auto permission =
      blink::PermissionDescriptorToPermissionType(permission_descriptor);
  base::DictValue details;
  details.Set("embeddingOrigin",
              content::PermissionUtil::GetLastCommittedOriginAsURL(
                  render_frame_host->GetMainFrame())
                  .spec());
  bool granted = CheckPermissionWithDetails(
      permission, render_frame_host,
      render_frame_host->GetLastCommittedOrigin().GetURL(), std::move(details));
  return granted ? content::PermissionResult(
                       blink::mojom::PermissionStatus::GRANTED)
                 : content::PermissionResult(
                       blink::mojom::PermissionStatus::DENIED);
}

content::PermissionResult
ElectronPermissionManager::GetPermissionResultForWorker(
    const blink::mojom::PermissionDescriptorPtr& permission_descriptor,
    content::RenderProcessHost* render_process_host,
    const GURL& worker_origin) {
  blink::mojom::PermissionStatus status =
      GetPermissionStatus(permission_descriptor, worker_origin, worker_origin);
  return content::PermissionResult(status);
}

content::PermissionResult
ElectronPermissionManager::GetPermissionResultForEmbeddedRequester(
    const blink::mojom::PermissionDescriptorPtr& permission_descriptor,
    content::RenderFrameHost* render_frame_host,
    const url::Origin& requesting_origin) {
  if (render_frame_host->IsNestedWithinFencedFrame())
    return content::PermissionResult(blink::mojom::PermissionStatus::DENIED);

  blink::mojom::PermissionStatus status =
      GetPermissionStatus(permission_descriptor, requesting_origin.GetURL(),
                          render_frame_host->GetLastCommittedOrigin().GetURL());
  return content::PermissionResult(status);
}

}  // namespace electron
