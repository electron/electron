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

// Forces to use dark theme on Linux.
inline constexpr std::string_view kDarkTheme = "darkTheme";

// Whether the window should be transparent.
inline constexpr std::string_view kTransparent = "transparent";

// Window type hint.
inline constexpr std::string_view kType = "type";

// Disable auto-hiding cursor.
inline constexpr std::string_view kDisableAutoHideCursor =
    "disableAutoHideCursor";

// Default browser window background color.
inline constexpr std::string_view kBackgroundColor = "backgroundColor";

// Whether the window should have a shadow.
inline constexpr std::string_view kHasShadow = "hasShadow";

// Browser window opacity
inline constexpr std::string_view kOpacity = "opacity";

// Whether the window can be activated.
inline constexpr std::string_view kFocusable = "focusable";

// The WebPreferences.
inline constexpr std::string_view kWebPreferences = "webPreferences";

// Add a vibrancy effect to the browser window
inline constexpr std::string_view kVibrancyType = "vibrancy";

// Add a vibrancy effect to the browser window.
inline constexpr std::string_view kBackgroundMaterial = "backgroundMaterial";

// Specify how the material appearance should reflect window activity state on
// macOS.
inline constexpr std::string_view kVisualEffectState = "visualEffectState";

inline constexpr std::string_view kTrafficLightPosition =
    "trafficLightPosition";
inline constexpr std::string_view kRoundedCorners = "roundedCorners";

inline constexpr std::string_view ktitleBarOverlay = "titleBarOverlay";

// The color to use as the theme and symbol colors respectively for Window
// Controls Overlay if enabled on Windows.
inline constexpr std::string_view kOverlayButtonColor = "color";
inline constexpr std::string_view kOverlaySymbolColor = "symbolColor";

// The custom height for Window Controls Overlay.
inline constexpr std::string_view kOverlayHeight = "height";

/// WebPreferences.

// The factor of which page should be zoomed.
inline constexpr std::string_view kZoomFactor = "zoomFactor";

// Script that will be loaded by guest WebContents before other scripts.
inline constexpr std::string_view kPreloadScript = "preload";

// Enable the node integration.
inline constexpr std::string_view kNodeIntegration = "nodeIntegration";

// Enable context isolation of Electron APIs and preload script
inline constexpr std::string_view kContextIsolation = "contextIsolation";

// Web runtime features.
inline constexpr std::string_view kExperimentalFeatures =
    "experimentalFeatures";

// Enable the rubber banding effect.
inline constexpr std::string_view kScrollBounce = "scrollBounce";

// Enable blink features.
inline constexpr std::string_view kEnableBlinkFeatures = "enableBlinkFeatures";

// Disable blink features.
inline constexpr std::string_view kDisableBlinkFeatures =
    "disableBlinkFeatures";

// Enable the node integration in WebWorker.
inline constexpr std::string_view kNodeIntegrationInWorker =
    "nodeIntegrationInWorker";

// Enable the web view tag.
inline constexpr std::string_view kWebviewTag = "webviewTag";

inline constexpr std::string_view kCustomArgs = "additionalArguments";

inline constexpr std::string_view kPlugins = "plugins";

inline constexpr std::string_view kSandbox = "sandbox";

inline constexpr std::string_view kWebSecurity = "webSecurity";

inline constexpr std::string_view kAllowRunningInsecureContent =
    "allowRunningInsecureContent";

inline constexpr std::string_view kOffscreen = "offscreen";

inline constexpr std::string_view kUseSharedTexture = "useSharedTexture";

inline constexpr std::string_view kNodeIntegrationInSubFrames =
    "nodeIntegrationInSubFrames";

// Disable window resizing when HTML Fullscreen API is activated.
inline constexpr std::string_view kDisableHtmlFullscreenWindowResize =
    "disableHtmlFullscreenWindowResize";

// Enables JavaScript support.
inline constexpr std::string_view kJavaScript = "javascript";

// Enables image support.
inline constexpr std::string_view kImages = "images";

// Make TextArea elements resizable.
inline constexpr std::string_view kTextAreasAreResizable =
    "textAreasAreResizable";

// Enables WebGL support.
inline constexpr std::string_view kWebGL = "webgl";

// Whether dragging and dropping a file or link onto the page causes a
// navigation.
inline constexpr std::string_view kNavigateOnDragDrop = "navigateOnDragDrop";

inline constexpr std::string_view kEnablePreferredSizeMode =
    "enablePreferredSizeMode";

inline constexpr std::string_view kHiddenPage = "hiddenPage";

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
inline constexpr std::string_view kSpellcheck = "spellcheck";
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
