// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_H_
#define ELECTRON_SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/unguessable_token.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/serial_delegate.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/device/public/mojom/serial.mojom-forward.h"
#include "shell/browser/electron_browser_context.h"
#include "third_party/blink/public/mojom/serial/serial.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace base {
class Value;
}

namespace electron {

extern const char kHidVendorIdKey[];
extern const char kHidProductIdKey[];

#if BUILDFLAG(IS_WIN)
extern const char kDeviceInstanceIdKey[];
#else
extern const char kVendorIdKey[];
extern const char kProductIdKey[];
extern const char kSerialNumberKey[];
#if BUILDFLAG(IS_MAC)
extern const char kUsbDriverKey[];
#endif  // BUILDFLAG(IS_MAC)
#endif  // BUILDFLAG(IS_WIN)

class SerialChooserContext : public KeyedService,
                             public device::mojom::SerialPortManagerClient {
 public:
  using PortObserver = content::SerialDelegate::Observer;

  SerialChooserContext();
  ~SerialChooserContext() override;

  // disable copy
  SerialChooserContext(const SerialChooserContext&) = delete;
  SerialChooserContext& operator=(const SerialChooserContext&) = delete;

  // ObjectPermissionContextBase::PermissionObserver:
  void OnPermissionRevoked(const url::Origin& origin);

  // Serial-specific interface for granting and checking permissions.
  void GrantPortPermission(const url::Origin& origin,
                           const device::mojom::SerialPortInfo& port,
                           content::RenderFrameHost* render_frame_host);
  bool HasPortPermission(const url::Origin& origin,
                         const device::mojom::SerialPortInfo& port,
                         content::RenderFrameHost* render_frame_host);
  static bool CanStorePersistentEntry(
      const device::mojom::SerialPortInfo& port);

  device::mojom::SerialPortManager* GetPortManager();

  void AddPortObserver(PortObserver* observer);
  void RemovePortObserver(PortObserver* observer);

  base::WeakPtr<SerialChooserContext> AsWeakPtr();

  bool is_initialized_ = false;

  // Map from port token to port info.
  std::map<base::UnguessableToken, device::mojom::SerialPortInfoPtr> port_info_;

  // SerialPortManagerClient implementation.
  void OnPortAdded(device::mojom::SerialPortInfoPtr port) override;
  void OnPortRemoved(device::mojom::SerialPortInfoPtr port) override;
  void RevokePortPermissionWebInitiated(const url::Origin& origin,
                                        const base::UnguessableToken& token);
  // Only call this if you're sure |port_info_| has been initialized
  // before-hand. The returned raw pointer is owned by |port_info_| and will be
  // destroyed when the port is removed.
  const device::mojom::SerialPortInfo* GetPortInfo(
      const base::UnguessableToken& token);

 private:
  void EnsurePortManagerConnection();
  void SetUpPortManagerConnection(
      mojo::PendingRemote<device::mojom::SerialPortManager> manager);
  void OnPortManagerConnectionError();
  void RevokeObjectPermissionInternal(const url::Origin& origin,
                                      const base::Value& object,
                                      bool revoked_by_website);

  mojo::Remote<device::mojom::SerialPortManager> port_manager_;
  mojo::Receiver<device::mojom::SerialPortManagerClient> client_receiver_{this};
  base::ObserverList<PortObserver> port_observer_list_;

  base::WeakPtrFactory<SerialChooserContext> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_H_
