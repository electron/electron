// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_H_
#define SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_H_

#include <map>
#include <memory>
#include <set>
#include <string>
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
#include "third_party/blink/public/mojom/serial/serial.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace base {
class Value;
}

namespace electron {

class SerialChooserContext : public KeyedService,
                             public device::mojom::SerialPortManagerClient {
 public:
  using PortObserver = content::SerialDelegate::Observer;

  SerialChooserContext();
  ~SerialChooserContext() override;

  // Serial-specific interface for granting and checking permissions.
  void GrantPortPermission(const url::Origin& requesting_origin,
                           const url::Origin& embedding_origin,
                           const device::mojom::SerialPortInfo& port);
  bool HasPortPermission(const url::Origin& requesting_origin,
                         const url::Origin& embedding_origin,
                         const device::mojom::SerialPortInfo& port);
  static bool CanStorePersistentEntry(
      const device::mojom::SerialPortInfo& port);

  device::mojom::SerialPortManager* GetPortManager();

  void AddPortObserver(PortObserver* observer);
  void RemovePortObserver(PortObserver* observer);

  base::WeakPtr<SerialChooserContext> AsWeakPtr();

  // SerialPortManagerClient implementation.
  void OnPortAdded(device::mojom::SerialPortInfoPtr port) override;
  void OnPortRemoved(device::mojom::SerialPortInfoPtr port) override;

 private:
  void EnsurePortManagerConnection();
  void SetUpPortManagerConnection(
      mojo::PendingRemote<device::mojom::SerialPortManager> manager);
  void OnPortManagerConnectionError();
  void OnGetPorts(const url::Origin& requesting_origin,
                  const url::Origin& embedding_origin,
                  blink::mojom::SerialService::GetPortsCallback callback,
                  std::vector<device::mojom::SerialPortInfoPtr> ports);

  // Tracks the set of ports to which an origin (potentially embedded in another
  // origin) has access to. Key is (requesting_origin, embedding_origin).
  std::map<std::pair<url::Origin, url::Origin>,
           std::set<base::UnguessableToken>>
      ephemeral_ports_;

  // Holds information about ports in |ephemeral_ports_|.
  std::map<base::UnguessableToken, base::Value> port_info_;

  mojo::Remote<device::mojom::SerialPortManager> port_manager_;
  mojo::Receiver<device::mojom::SerialPortManagerClient> client_receiver_{this};
  base::ObserverList<PortObserver> port_observer_list_;

  base::WeakPtrFactory<SerialChooserContext> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(SerialChooserContext);
};

}  // namespace electron

#endif  // SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_H_
