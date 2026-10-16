// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_SHIM_APP_SHIM_APPLICATION_H_
#define ELECTRON_SHELL_APP_SHIM_APP_SHIM_APPLICATION_H_

#import <Cocoa/Cocoa.h>

#import "components/remote_cocoa/app_shim/cr_application.h"

// The NSApplication of an app shim process. It notices assistive technologies
// (VoiceOver, Accessibility Inspector, UI scripting) and has the host app turn
// accessibility on, because the accessibility tree of the windows shown here
// is built there.
@interface ElectronAppShimApplication : CrApplication
@end

#endif  // ELECTRON_SHELL_APP_SHIM_APP_SHIM_APPLICATION_H_
