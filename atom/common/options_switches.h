// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

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
extern const char kAlwaysOnTop[];
extern const char kAcceptFirstMouse[];
extern const char kUseContentSize[];
extern const char kTitleBarStyle[];
extern const char kAutoHideMenuBar[];
extern const char kEnableLargerThanScreen[];
extern const char kDarkTheme[];
extern const char kTransparent[];
extern const char kType[];
extern const char kDisableAutoHideCursor[];
extern const char kStandardWindow[];
extern const char kBackgroundColor[];
extern const char kHasShadow[];
extern const char kWebPreferences[];

// WebPreferences.
extern const char kDirectWrite[];
extern const char kZoomFactor[];
extern const char kPreloadScript[];
extern const char kPreloadURL[];
extern const char kNodeIntegration[];
extern const char kGuestInstanceID[];
extern const char kExperimentalFeatures[];
extern const char kExperimentalCanvasFeatures[];
extern const char kOpenerID[];
extern const char kScrollBounce[];
extern const char kBlinkFeatures[];
extern const char kDisableBlinkFeatures[];

}   // namespace options


// Following are actually command line switches, should be moved to other files.

namespace switches {

extern const char kEnablePlugins[];
extern const char kPpapiFlashPath[];
extern const char kPpapiFlashVersion[];
extern const char kDisableHttpCache[];
extern const char kStandardSchemes[];
extern const char kRegisterServiceWorkerSchemes[];
extern const char kSSLVersionFallbackMin[];
extern const char kCipherSuiteBlacklist[];
extern const char kAppUserModelId[];

extern const char kBackgroundColor[];
extern const char kZoomFactor[];
extern const char kPreloadScript[];
extern const char kPreloadURL[];
extern const char kNodeIntegration[];
extern const char kGuestInstanceID[];
extern const char kOpenerID[];
extern const char kScrollBounce[];

extern const char kWidevineCdmPath[];
extern const char kWidevineCdmVersion[];

}  // namespace switches

}  // namespace atom
