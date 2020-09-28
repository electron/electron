// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/serial/serial_chooser_context.h"

#include <utility>

#include "base/base64.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/device_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace electron {

constexpr char kPortNameKey[] = "name";
constexpr char kPersistentIdKey[] = "persistent_id";
constexpr char kTokenKey[] = "token";

std::string EncodeToken(const base::UnguessableToken& token) {
  const uint64_t data[2] = {token.GetHighForSerialization(),
                            token.GetLowForSerialization()};
  std::string buffer;
  base::Base64Encode(
      base::StringPiece(reinterpret_cast<const char*>(&data[0]), sizeof(data)),
      &buffer);
  return buffer;
}

base::UnguessableToken DecodeToken(base::StringPiece input) {
  std::string buffer;
  if (!base::Base64Decode(input, &buffer) ||
      buffer.length() != sizeof(uint64_t) * 2) {
    return base::UnguessableToken();
  }

  const uint64_t* data = reinterpret_cast<const uint64_t*>(buffer.data());
  return base::UnguessableToken::Deserialize(data[0], data[1]);
}

base::Value PortInfoToValue(const device::mojom::SerialPortInfo& port) {
  base::Value value(base::Value::Type::DICTIONARY);
  if (port.display_name && !port.display_name->empty())
    value.SetStringKey(kPortNameKey, *port.display_name);
  else
    value.SetStringKey(kPortNameKey, port.path.LossyDisplayName());
  if (SerialChooserContext::CanStorePersistentEntry(port))
    value.SetStringKey(kPersistentIdKey, port.persistent_id.value());
  else
    value.SetStringKey(kTokenKey, EncodeToken(port.token));
  return value;
}

SerialChooserContext::SerialChooserContext() = default;
SerialChooserContext::~SerialChooserContext() = default;

void SerialChooserContext::GrantPortPermission(
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin,
    const device::mojom::SerialPortInfo& port) {
  base::Value value = PortInfoToValue(port);
  port_info_.insert({port.token, value.Clone()});

  ephemeral_ports_[{requesting_origin, embedding_origin}].insert(port.token);
}

bool SerialChooserContext::HasPortPermission(
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin,
    const device::mojom::SerialPortInfo& port) {
  auto it = ephemeral_ports_.find({requesting_origin, embedding_origin});
  if (it != ephemeral_ports_.end()) {
    const std::set<base::UnguessableToken> ports = it->second;
    if (base::Contains(ports, port.token))
      return true;
  }

  return false;
}

// static
bool SerialChooserContext::CanStorePersistentEntry(
    const device::mojom::SerialPortInfo& port) {
  // If there is no display name then the path name will be used instead. The
  // path name is not guaranteed to be stable. For example, on Linux the name
  // "ttyUSB0" is reused for any USB serial device. A name like that would be
  // confusing to show in settings when the device is disconnected.
  if (!port.display_name || port.display_name->empty())
    return false;

  return port.persistent_id && !port.persistent_id->empty();
}

device::mojom::SerialPortManager* SerialChooserContext::GetPortManager() {
  EnsurePortManagerConnection();
  return port_manager_.get();
}

void SerialChooserContext::AddPortObserver(PortObserver* observer) {
  port_observer_list_.AddObserver(observer);
}

void SerialChooserContext::RemovePortObserver(PortObserver* observer) {
  port_observer_list_.RemoveObserver(observer);
}

base::WeakPtr<SerialChooserContext> SerialChooserContext::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void SerialChooserContext::OnPortAdded(device::mojom::SerialPortInfoPtr port) {
  for (auto& observer : port_observer_list_)
    observer.OnPortAdded(*port);
}

void SerialChooserContext::OnPortRemoved(
    device::mojom::SerialPortInfoPtr port) {
  for (auto& observer : port_observer_list_)
    observer.OnPortRemoved(*port);

  std::vector<std::pair<url::Origin, url::Origin>> revoked_url_pairs;
  for (auto& map_entry : ephemeral_ports_) {
    std::set<base::UnguessableToken>& ports = map_entry.second;
    if (ports.erase(port->token) > 0) {
      revoked_url_pairs.push_back(map_entry.first);
    }
  }

  port_info_.erase(port->token);
}

void SerialChooserContext::EnsurePortManagerConnection() {
  if (port_manager_)
    return;

  mojo::PendingRemote<device::mojom::SerialPortManager> manager;
  content::GetDeviceService().BindSerialPortManager(
      manager.InitWithNewPipeAndPassReceiver());
  SetUpPortManagerConnection(std::move(manager));
}

void SerialChooserContext::SetUpPortManagerConnection(
    mojo::PendingRemote<device::mojom::SerialPortManager> manager) {
  port_manager_.Bind(std::move(manager));
  port_manager_.set_disconnect_handler(
      base::BindOnce(&SerialChooserContext::OnPortManagerConnectionError,
                     base::Unretained(this)));

  port_manager_->SetClient(client_receiver_.BindNewPipeAndPassRemote());
}

void SerialChooserContext::OnPortManagerConnectionError() {
  port_manager_.reset();
  client_receiver_.reset();

  port_info_.clear();

  ephemeral_ports_.clear();
}

}  // namespace electron
