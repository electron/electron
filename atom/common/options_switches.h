// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_OPTIONS_SWITCHES_H_
#define ATOM_COMMON_OPTIONS_SWITCHES_H_

namespace atom {

namespace options {

extern const char kTitle[];
extern const char kIcon[];
extern const char kFrame[];
extern const char kShow[];
extern const char kCenter[];
extern const char kX[];
extern const char kY[];
extern const char kWidth[];
extern const char kHeight[];
extern const char kMinWidth[];
extern const char kMinHeight[];
extern const char kMaxWidth[];
extern const char kMaxHeight[];
extern const char kResizable[];
extern const char kMovable[];
extern const char kMinimizable[];
extern const char kMaximizable[];
extern const char kFullScreenable[];
extern const char kClosable[];
extern const char kFullscreen[];
extern const char kSkipTaskbar[];
extern const char kKiosk[];
extern const char kSimpleFullScreen[];
extern const char kAlwaysOnTop[];
extern const char kAcceptFirstMouse[];
extern const char kUseContentSize[];
extern const char kZoomToPageWidth[];
extern const char kFullscreenWindowTitle[];
extern const char kTitleBarStyle[];
extern const char kTabbingIdentifier[];
extern const char kAutoHideMenuBar[];
extern const char kEnableLargerThanScreen[];
extern const char kDarkTheme[];
extern const char kTransparent[];
extern const char kType[];
extern const char kDisableAutoHideCursor[];
extern const char kStandardWindow[];
extern const char kBackgroundColor[];
extern const char kHasShadow[];
extern const char kOpacity[];
extern const char kFocusable[];
extern const char kWebPreferences[];
extern const char kVibrancyType[];

// WebPreferences.
extern const char kZoomFactor[];
extern const char kPreloadScript[];
extern const char kPreloadURL[];
extern const char kNodeIntegration[];
extern const char kEnableRemoteModule[];
extern const char kContextIsolation[];
extern const char kGuestInstanceID[];
extern const char kExperimentalFeatures[];
extern const char kOpenerID[];
extern const char kScrollBounce[];
extern const char kEnableBlinkFeatures[];
extern const char kDisableBlinkFeatures[];
extern const char kNodeIntegrationInWorker[];
extern const char kWebviewTag[];
extern const char kNativeWindowOpen[];
extern const char kCustomArgs[];
extern const char kPlugins[];
extern const char kSandbox[];
extern const char kWebSecurity[];
extern const char kAllowRunningInsecureContent[];
extern const char kOffscreen[];

}  // namespace options

// Following are actually command line switches, should be moved to other files.

namespace switches {

extern const char kEnableSandbox[];
extern const char kEnableMixedSandbox[];
extern const char kEnablePlugins[];
extern const char kPpapiFlashPath[];
extern const char kPpapiFlashVersion[];
extern const char kDisableHttpCache[];
extern const char kStandardSchemes[];
extern const char kRegisterServiceWorkerSchemes[];
extern const char kSecureSchemes[];
extern const char kAppUserModelId[];
extern const char kAppPath[];

extern const char kBackgroundColor[];
extern const char kPreloadScript[];
extern const char kPreloadScripts[];
extern const char kNodeIntegration[];
extern const char kDisableRemoteModule[];
extern const char kContextIsolation[];
extern const char kGuestInstanceID[];
extern const char kOpenerID[];
extern const char kScrollBounce[];
extern const char kHiddenPage[];
extern const char kNativeWindowOpen[];
extern const char kNodeIntegrationInWorker[];
extern const char kWebviewTag[];

extern const char kWidevineCdmPath[];
extern const char kWidevineCdmVersion[];

extern const char kDiskCacheSize[];
extern const char kIgnoreConnectionsLimit[];
extern const char kAuthServerWhitelist[];
extern const char kAuthNegotiateDelegateWhitelist[];

}  // namespace switches

}  // namespace atom

#endif  // ATOM_COMMON_OPTIONS_SWITCHES_H_
