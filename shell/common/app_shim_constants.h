// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_APP_SHIM_CONSTANTS_H_
#define ELECTRON_SHELL_COMMON_APP_SHIM_CONSTANTS_H_

#include <mach/message.h>

#include <string>

namespace electron::app_shim {

// The --type of an app shim process. It must be the type that the sandbox
// policy lists as an unsandboxed process (sandbox::policy::switches::
// kAppShimProcessType), or content refuses to start it.
inline constexpr char kProcessType[] = "app-shim";

// Mach message ID a shim sends to the browser's bootstrap server to connect.
inline constexpr mach_msg_id_t kBootstrapMsgId = 'eshm';

// Mach message ID a shim sends to that server, in place of connecting, when
// it finds that the browser is another build of Electron than the one it
// loaded. The message has the layout of the one above and no channel.
inline constexpr mach_msg_id_t kBuildMismatchMsgId = 'eshv';

// The name of a second bootstrap server that the browser publishes beside
// `server_name` and never reads from. It is there for a shim to look up: the
// shim loaded the Electron Framework that is on disk, the browser is running
// the one that was on disk when it started, and when the app has been
// replaced in between, the two can't talk Mojo to each other. The name holds
// the build of the framework, so a shim finds it only when the builds match.
//
// TODO(MarshallOfSound): keep each version's framework in a folder of its own
// (Versions/<version>, as Chrome does) and have the shim load the one that
// the running browser uses. A mismatch can't happen then, and this goes away.
std::string BuildMarkerServerName(const std::string& server_name);

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
