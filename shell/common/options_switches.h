// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_OPTIONS_SWITCHES_H_
#define ELECTRON_SHELL_COMMON_OPTIONS_SWITCHES_H_

#include <string_view>

#include "electron/buildflags/buildflags.h"

namespace electron {

namespace options {

inline constexpr std::string_view kTitle = "title";
inline constexpr std::string_view kIcon = "icon";
inline constexpr std::string_view kFrame = "frame";
inline constexpr std::string_view kShow = "show";
inline constexpr std::string_view kCenter = "center";
inline constexpr std::string_view kX = "x";
inline constexpr std::string_view kY = "y";
inline constexpr std::string_view kWidth = "width";
inline constexpr std::string_view kHeight = "height";
inline constexpr std::string_view kMinWidth = "minWidth";
inline constexpr std::string_view kMinHeight = "minHeight";
inline constexpr std::string_view kMaxWidth = "maxWidth";
inline constexpr std::string_view kMaxHeight = "maxHeight";
inline constexpr std::string_view kResizable = "resizable";
inline constexpr std::string_view kMovable = "movable";
inline constexpr std::string_view kMinimizable = "minimizable";
inline constexpr std::string_view kMaximizable = "maximizable";
inline constexpr std::string_view kFullScreenable = "fullscreenable";
inline constexpr std::string_view kClosable = "closable";

// whether to keep the window out of mission control
inline constexpr std::string_view kHiddenInMissionControl =
    "hiddenInMissionControl";

inline constexpr std::string_view kFullscreen = "fullscreen";

// Whether the window should show in taskbar.
inline constexpr std::string_view kSkipTaskbar = "skipTaskbar";

// Start with the kiosk mode, see Opera's page for description:
// http://www.opera.com/support/mastering/kiosk/
inline constexpr std::string_view kKiosk = "kiosk";

inline constexpr std::string_view kSimpleFullScreen = "simpleFullscreen";

// Make windows stays on the top of all other windows.
inline constexpr std::string_view kAlwaysOnTop = "alwaysOnTop";

// Enable the NSView to accept first mouse event.
inline constexpr std::string_view kAcceptFirstMouse = "acceptFirstMouse";

// Whether window size should include window frame.
inline constexpr std::string_view kUseContentSize = "useContentSize";

// Whether window zoom should be to page width.
inline constexpr std::string_view kZoomToPageWidth = "zoomToPageWidth";

// The requested title bar style for the window
inline constexpr std::string_view kTitleBarStyle = "titleBarStyle";

// Tabbing identifier for the window if native tabs are enabled on macOS.
inline constexpr std::string_view kTabbingIdentifier = "tabbingIdentifier";

// The menu bar is hidden unless "Alt" is pressed.
inline constexpr std::string_view kAutoHideMenuBar = "autoHideMenuBar";

// Enable window to be resized larger than screen.
inline constexpr std::string_view kEnableLargerThanScreen =
    "enableLargerThanScreen";

extern const char kDarkTheme[];
extern const char kTransparent[];
extern const char kType[];
extern const char kDisableAutoHideCursor[];
extern const char kBackgroundColor[];
extern const char kHasShadow[];
extern const char kOpacity[];
extern const char kFocusable[];
extern const char kWebPreferences[];
extern const char kVibrancyType[];
extern const char kBackgroundMaterial[];
extern const char kVisualEffectState[];
extern const char kTrafficLightPosition[];
extern const char kRoundedCorners[];
extern const char ktitleBarOverlay[];
extern const char kOverlayButtonColor[];
extern const char kOverlaySymbolColor[];
extern const char kOverlayHeight[];

// WebPreferences.
extern const char kZoomFactor[];
extern const char kPreloadScript[];
extern const char kNodeIntegration[];
extern const char kContextIsolation[];
extern const char kExperimentalFeatures[];
extern const char kScrollBounce[];
extern const char kEnableBlinkFeatures[];
extern const char kDisableBlinkFeatures[];
extern const char kNodeIntegrationInWorker[];
extern const char kWebviewTag[];
extern const char kCustomArgs[];
extern const char kPlugins[];
extern const char kSandbox[];
extern const char kWebSecurity[];
extern const char kAllowRunningInsecureContent[];
extern const char kOffscreen[];
extern const char kUseSharedTexture[];
extern const char kNodeIntegrationInSubFrames[];
extern const char kDisableHtmlFullscreenWindowResize[];
extern const char kJavaScript[];
extern const char kImages[];
extern const char kTextAreasAreResizable[];
extern const char kWebGL[];
extern const char kNavigateOnDragDrop[];
extern const char kEnablePreferredSizeMode[];

extern const char kHiddenPage[];

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
extern const char kSpellcheck[];
#endif

}  // namespace options

// Following are actually command line switches, should be moved to other files.

namespace switches {

extern const char kEnableSandbox[];
extern const char kDisableHttpCache[];
extern const char kStandardSchemes[];
extern const char kServiceWorkerSchemes[];
extern const char kSecureSchemes[];
extern const char kBypassCSPSchemes[];
extern const char kFetchSchemes[];
extern const char kCORSSchemes[];
extern const char kStreamingSchemes[];
extern const char kCodeCacheSchemes[];
extern const char kAppUserModelId[];
extern const char kAppPath[];

extern const char kScrollBounce[];
extern const char kNodeIntegrationInWorker[];

extern const char kWidevineCdmPath[];
extern const char kWidevineCdmVersion[];

extern const char kDiskCacheSize[];
extern const char kIgnoreConnectionsLimit[];
extern const char kAuthServerWhitelist[];
extern const char kAuthNegotiateDelegateWhitelist[];
extern const char kEnableAuthNegotiatePort[];
extern const char kDisableNTLMv2[];
}  // namespace switches

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_OPTIONS_SWITCHES_H_
