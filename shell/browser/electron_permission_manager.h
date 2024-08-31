// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_PERMISSION_MANAGER_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_PERMISSION_MANAGER_H_

#include <memory>
#include <vector>

#include "base/containers/id_map.h"
#include "base/functional/callback_forward.h"
#include "base/values.h"
#include "content/public/browser/permission_controller_delegate.h"

namespace content {
class WebContents;
}

namespace gin_helper {
class Dictionary;
}  // namespace gin_helper

namespace v8 {
class Object;
template <typename T>
class Local;
}  // namespace v8

namespace electron {

class ElectronBrowserContext;

class ElectronPermissionManager : public content::PermissionControllerDelegate {
 public:
  ElectronPermissionManager();
  ~ElectronPermissionManager() override;

  // disable copy
  ElectronPermissionManager(const ElectronPermissionManager&) = delete;
  ElectronPermissionManager& operator=(const ElectronPermissionManager&) =
      delete;

  using USBProtectedClasses = std::vector<uint8_t>;

  using StatusCallback =
      base::OnceCallback<void(blink::mojom::PermissionStatus)>;
  using StatusesCallback = base::OnceCallback<void(
      const std::vector<blink::mojom::PermissionStatus>&)>;
  using PairCallback = base::OnceCallback<void(base::Value::Dict)>;
  using OnRequestHandler = base::RepeatingCallback<void(content::WebContents*,
                                                      blink::PermissionType,
                                                      StatusCallback,
                                                      const base::Value&,
                                                      const url::Origin& effective_origin)>;
  using IsGrantedHandler =
      base::RepeatingCallback<std::optional<blink::mojom::PermissionStatus>(content::WebContents*,
                                   blink::PermissionType,
                                   const url::Origin& effective_origin,
                                   const base::Value&)>;

  using DeviceCheckHandler =
      base::RepeatingCallback<bool(const v8::Local<v8::Object>&)>;

  using ProtectedUSBHandler = base::RepeatingCallback<USBProtectedClasses(
      const v8::Local<v8::Object>&)>;

  using BluetoothPairingHandler =
      base::RepeatingCallback<void(gin_helper::Dictionary, PairCallback)>;

  // Handlers to dispatch permission requests in JS.
  void SetPermissionRequestHandler(const OnRequestHandler& handler);
  void SetPermissionIsGrantedHandler(const IsGrantedHandler& handler);
  void SetDevicePermissionHandler(const DeviceCheckHandler& handler);
  void SetProtectedUSBHandler(const ProtectedUSBHandler& handler);
  void SetBluetoothPairingHandler(const BluetoothPairingHandler& handler);

  // Bluetooth permissions, maps to session.setBluetoothPairingHandler()
  void CheckBluetoothDevicePair(gin_helper::Dictionary details,
                                PairCallback pair_callback) const;

  // Device permissions, maps to session.setDevicePermissionHandler()
  bool CheckDevicePermission(blink::PermissionType permission,
                             const url::Origin& origin,
                             const base::Value& object,
                             ElectronBrowserContext* browser_context) const;
  void GrantDevicePermission(blink::PermissionType permission,
                             const url::Origin& origin,
                             const base::Value& object,
                             ElectronBrowserContext* browser_context) const;
  void RevokeDevicePermission(blink::PermissionType permission,
                              const url::Origin& origin,
                              const base::Value& object,
                              ElectronBrowserContext* browser_context) const;

  // USB permissions, maps to session.setUSBProtectedClassesHandler()
  USBProtectedClasses CheckProtectedUSBClasses(
      const USBProtectedClasses& classes) const;

  // Permission granted checks, maps to session.setPermissionHandlers({ isGranted })
  blink::mojom::PermissionStatus CheckPermissionWithDetails(blink::PermissionType permission,
                                  const url::Origin& effective_origin,
                                  base::Value::Dict details) const;
  blink::mojom::PermissionStatus CheckPermissionWithDetailsAndFrame(blink::PermissionType permission,
                                          const url::Origin& effective_origin,
                                          content::RenderFrameHost* requesting_frame,
                                          base::Value::Dict details) const;

  // Permission requests, maps to session.setPermissionHandlers({ onRequest })
  void RequestPermissionWithDetails(blink::PermissionType permission,
                                    content::RenderFrameHost* render_frame_host,
                                    const url::Origin& effective_origin,
                                    bool user_gesture,
                                    base::Value::Dict details,
                                    StatusCallback response_callback);

 protected:
  void OnPermissionResponse(int request_id,
                            int permission_id,
                            blink::mojom::PermissionStatus status);

  // content::PermissionControllerDelegate:
  void RequestPermissions(
      content::RenderFrameHost* render_frame_host,
      const content::PermissionRequestDescription& request_description,
      StatusesCallback callback) override;
  void ResetPermission(blink::PermissionType permission,
                       const GURL& effective_origin,
                       const GURL& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatus(
      blink::PermissionType permission,
      const GURL& effective_origin,
      const GURL& embedding_origin) override;
  void RequestPermissionsFromCurrentDocument(
      content::RenderFrameHost* render_frame_host,
      const content::PermissionRequestDescription& request_description,
      base::OnceCallback<
          void(const std::vector<blink::mojom::PermissionStatus>&)> callback)
      override;
  content::PermissionResult GetPermissionResultForOriginWithoutContext(
      blink::PermissionType permission,
      const url::Origin& effective_origin,
      const url::Origin& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatusForCurrentDocument(
      blink::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      bool should_include_device_status) override;
  blink::mojom::PermissionStatus GetPermissionStatusForWorker(
      blink::PermissionType permission,
      content::RenderProcessHost* render_process_host,
      const GURL& worker_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatusForEmbeddedRequester(
      blink::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const url::Origin& effective_origin) override;
  SubscriptionId SubscribeToPermissionStatusChange(
      blink::PermissionType permission,
      content::RenderProcessHost* render_process_host,
      content::RenderFrameHost* render_frame_host,
      const GURL& effective_origin,
      bool should_include_device_status,
      base::RepeatingCallback<void(blink::mojom::PermissionStatus)> callback)
      override;
  void UnsubscribeFromPermissionStatusChange(SubscriptionId id) override;

 private:
  class PendingRequest;
  using PendingRequestsMap = base::IDMap<std::unique_ptr<PendingRequest>>;

  void RequestPermissionsWithDetails(
      content::RenderFrameHost* render_frame_host,
      const content::PermissionRequestDescription& request_description,
      base::Value::Dict details,
      StatusesCallback callback);

  OnRequestHandler on_request_handler_;
  IsGrantedHandler is_granted_handler_;
  DeviceCheckHandler device_permission_handler_;
  ProtectedUSBHandler protected_usb_handler_;
  BluetoothPairingHandler bluetooth_pairing_handler_;

  PendingRequestsMap pending_requests_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_PERMISSION_MANAGER_H_
