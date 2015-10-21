// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_OPTIONS_SWITCHES_H_
#define ATOM_COMMON_OPTIONS_SWITCHES_H_

namespace atom {

namespace switches {

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
extern const char kFullscreen[];
extern const char kSkipTaskbar[];
extern const char kKiosk[];
extern const char kAlwaysOnTop[];
extern const char kNodeIntegration[];
extern const char kAcceptFirstMouse[];
extern const char kUseContentSize[];
extern const char kTitleBarStyle[];
extern const char kWebPreferences[];
extern const char kZoomFactor[];
extern const char kAutoHideMenuBar[];
extern const char kEnableLargerThanScreen[];
extern const char kDarkTheme[];
extern const char kDirectWrite[];
extern const char kEnablePlugins[];
extern const char kPpapiFlashPath[];
extern const char kPpapiFlashVersion[];
extern const char kGuestInstanceID[];
extern const char kPreloadScript[];
extern const char kPreloadUrl[];
extern const char kTransparent[];
extern const char kType[];
extern const char kDisableAutoHideCursor[];
extern const char kStandardWindow[];
extern const char kBackgroundColor[];
extern const char kClientCertificate[];

extern const char kExperimentalFeatures[];
extern const char kExperimentalCanvasFeatures[];
extern const char kOverlayScrollbars[];
extern const char kOverlayFullscreenVideo[];
extern const char kSharedWorker[];
extern const char kPageVisibility[];

extern const char kDisableHttpCache[];
extern const char kRegisterStandardSchemes[];
extern const char kSSLVersionFallbackMin[];
extern const char kCipherSuiteBlacklist[];

extern const char kAppUserModelId[];

}  // namespace switches

}  // namespace atom

#endif  // ATOM_COMMON_OPTIONS_SWITCHES_H_
