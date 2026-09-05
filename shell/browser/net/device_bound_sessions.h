// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_DEVICE_BOUND_SESSIONS_H_
#define ELECTRON_SHELL_BROWSER_NET_DEVICE_BOUND_SESSIONS_H_

#include <memory>

namespace base {
class FilePath;
}  // namespace base

namespace unexportable_keys {
class UnexportableKeyService;
}  // namespace unexportable_keys

namespace electron {

// Whether Device Bound Session Credentials should be configured on new network
// contexts.
bool ShouldEnableDeviceBoundSessions();

// Creates the `UnexportableKeyService` that DBSC signs with for the browser
// context rooted at `context_path`, or nullptr when this platform or this app
// cannot provide unexportable keys.
//
// The network service has its own fallback factory, but on macOS that one
// hardcodes Chromium's keychain access group -- which no Electron app can hold
// an entitlement for -- and it runs key operations inside the sandboxed network
// process. Electron creates the service in the browser process instead and
// hands the network service a remote to it. On Linux the network service keeps
// using its own factory, because
// network::features::kUseUnexportableKeyServiceInBrowserProcess is disabled by
// default there.
//
// One service per browser context, not one per process: on macOS the config
// carries an `application_tag` derived from `context_path`, and that tag scopes
// DBSC's periodic garbage collection. A collector deletes every key in the
// access group whose tag matches its prefix and whose wrapped key is not in its
// own store, so two sessions sharing a tag would delete each other's keys.
std::unique_ptr<unexportable_keys::UnexportableKeyService>
CreateDeviceBoundSessionsKeyService(const base::FilePath& context_path);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_DEVICE_BOUND_SESSIONS_H_
