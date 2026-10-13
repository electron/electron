// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_APP_SHIM_CONSTANTS_H_
#define ELECTRON_SHELL_COMMON_APP_SHIM_CONSTANTS_H_

#include <mach/message.h>

namespace electron::app_shim {

// The --type of an app shim process. It must be the type that the sandbox
// policy lists as an unsandboxed process (sandbox::policy::switches::
// kAppShimProcessType), or content refuses to start it.
inline constexpr char kProcessType[] = "app-shim";

// Mach message ID a shim sends to the browser's bootstrap server to connect.
inline constexpr mach_msg_id_t kBootstrapMsgId = 'eshm';

// The browser publishes its bootstrap server as
// "<base bundle id>.<name>". This is the default name.
inline constexpr char kDefaultServerName[] = "appshim";

// Keys in the shim bundle's Info.plist.
// The name the browser passed to app.startAppShimServer(), when not the
// default.
inline constexpr char kInfoPlistServerNameKey[] = "ElectronAppShimServerName";
// Arguments to start the host app with when it is not running. An array of
// strings.
inline constexpr char kInfoPlistHostArgumentsKey[] =
    "ElectronAppShimHostArguments";

}  // namespace electron::app_shim

#endif  // ELECTRON_SHELL_COMMON_APP_SHIM_CONSTANTS_H_
