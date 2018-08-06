// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/options_switches.h"

namespace atom {

namespace options {

const char kTitle[] = "title";
const char kIcon[] = "icon";
const char kFrame[] = "frame";
const char kShow[] = "show";
const char kCenter[] = "center";
const char kX[] = "x";
const char kY[] = "y";
const char kWidth[] = "width";
const char kHeight[] = "height";
const char kMinWidth[] = "minWidth";
const char kMinHeight[] = "minHeight";
const char kMaxWidth[] = "maxWidth";
const char kMaxHeight[] = "maxHeight";
const char kResizable[] = "resizable";
const char kMovable[] = "movable";
const char kMinimizable[] = "minimizable";
const char kMaximizable[] = "maximizable";
const char kFullScreenable[] = "fullscreenable";
const char kClosable[] = "closable";
const char kFullscreen[] = "fullscreen";

// Whether the window should show in taskbar.
const char kSkipTaskbar[] = "skipTaskbar";

// Start with the kiosk mode, see Opera's page for description:
// http://www.opera.com/support/mastering/kiosk/
const char kKiosk[] = "kiosk";

const char kSimpleFullScreen[] = "simpleFullscreen";

// Make windows stays on the top of all other windows.
const char kAlwaysOnTop[] = "alwaysOnTop";

// Enable the NSView to accept first mouse event.
const char kAcceptFirstMouse[] = "acceptFirstMouse";

// Whether window size should include window frame.
const char kUseContentSize[] = "useContentSize";

// Whether window zoom should be to page width.
const char kZoomToPageWidth[] = "zoomToPageWidth";

// Whether always show title text in full screen is enabled.
const char kFullscreenWindowTitle[] = "fullscreenWindowTitle";

// The requested title bar style for the window
const char kTitleBarStyle[] = "titleBarStyle";

// Tabbing identifier for the window if native tabs are enabled on macOS.
const char kTabbingIdentifier[] = "tabbingIdentifier";

// The menu bar is hidden unless "Alt" is pressed.
const char kAutoHideMenuBar[] = "autoHideMenuBar";

// Enable window to be resized larger than screen.
const char kEnableLargerThanScreen[] = "enableLargerThanScreen";

// Forces to use dark theme on Linux.
const char kDarkTheme[] = "darkTheme";

// Whether the window should be transparent.
const char kTransparent[] = "transparent";

// Window type hint.
const char kType[] = "type";

// Disable auto-hiding cursor.
const char kDisableAutoHideCursor[] = "disableAutoHideCursor";

// Use the macOS' standard window instead of the textured window.
const char kStandardWindow[] = "standardWindow";

// Default browser window background color.
const char kBackgroundColor[] = "backgroundColor";

// Whether the window should have a shadow.
const char kHasShadow[] = "hasShadow";

// Browser window opacity
const char kOpacity[] = "opacity";

// Whether the window can be activated.
const char kFocusable[] = "focusable";

// The WebPreferences.
const char kWebPreferences[] = "webPreferences";

// Add a vibrancy effect to the browser window
const char kVibrancyType[] = "vibrancy";

// The factor of which page should be zoomed.
const char kZoomFactor[] = "zoomFactor";

// Script that will be loaded by guest WebContents before other scripts.
const char kPreloadScript[] = "preload";

// Like --preload, but the passed argument is an URL.
const char kPreloadURL[] = "preloadURL";

// Enable the node integration.
const char kNodeIntegration[] = "nodeIntegration";

// Enable context isolation of Electron APIs and preload script
const char kContextIsolation[] = "contextIsolation";

// Instance ID of guest WebContents.
const char kGuestInstanceID[] = "guestInstanceId";

// Web runtime features.
const char kExperimentalFeatures[] = "experimentalFeatures";

// Opener window's ID.
const char kOpenerID[] = "openerId";

// Enable the rubber banding effect.
const char kScrollBounce[] = "scrollBounce";

// Enable blink features.
const char kEnableBlinkFeatures[] = "enableBlinkFeatures";

// Disable blink features.
const char kDisableBlinkFeatures[] = "disableBlinkFeatures";

// Enable the node integration in WebWorker.
const char kNodeIntegrationInWorker[] = "nodeIntegrationInWorker";

// Enable the web view tag.
const char kWebviewTag[] = "webviewTag";

const char kNativeWindowOpen[] = "nativeWindowOpen";

const char kCustomArgs[] = "additionalArguments";

const char kPlugins[] = "plugins";

const char kSandbox[] = "sandbox";

const char kWebSecurity[] = "webSecurity";

const char kAllowRunningInsecureContent[] = "allowRunningInsecureContent";

const char kOffscreen[] = "offscreen";

}  // namespace options

namespace switches {

// Enable chromium sandbox.
const char kEnableSandbox[] = "enable-sandbox";

// Enable sandbox in only remote content windows.
const char kEnableMixedSandbox[] = "enable-mixed-sandbox";

// Enable plugins.
const char kEnablePlugins[] = "enable-plugins";

// Ppapi Flash path.
const char kPpapiFlashPath[] = "ppapi-flash-path";

// Ppapi Flash version.
const char kPpapiFlashVersion[] = "ppapi-flash-version";

// The list of standard schemes.
const char kStandardSchemes[] = "standard-schemes";

// Register schemes to handle service worker.
const char kRegisterServiceWorkerSchemes[] = "register-service-worker-schemes";

// Register schemes as secure.
const char kSecureSchemes[] = "secure-schemes";

// The browser process app model ID
const char kAppUserModelId[] = "app-user-model-id";

// The application path
const char kAppPath[] = "app-path";

// The command line switch versions of the options.
const char kBackgroundColor[] = "background-color";
const char kPreloadScript[] = "preload";
const char kPreloadURL[] = "preload-url";
const char kPreloadScripts[] = "preload-scripts";
const char kNodeIntegration[] = "node-integration";
const char kContextIsolation[] = "context-isolation";
const char kGuestInstanceID[] = "guest-instance-id";
const char kOpenerID[] = "opener-id";
const char kScrollBounce[] = "scroll-bounce";
const char kHiddenPage[] = "hidden-page";
const char kNativeWindowOpen[] = "native-window-open";
const char kWebviewTag[] = "webview-tag";

// Command switch passed to renderer process to control nodeIntegration.
const char kNodeIntegrationInWorker[] = "node-integration-in-worker";

// Widevine options
// Path to Widevine CDM binaries.
const char kWidevineCdmPath[] = "widevine-cdm-path";
// Widevine CDM version.
const char kWidevineCdmVersion[] = "widevine-cdm-version";

// Comma-separated list of rules that control how hostnames are mapped.
//
// For example:
//    "MAP * 127.0.0.1" --> Forces all hostnames to be mapped to 127.0.0.1
//    "MAP *.google.com proxy" --> Forces all google.com subdomains to be
//                                 resolved to "proxy".
//    "MAP test.com [::1]:77 --> Forces "test.com" to resolve to IPv6 loopback.
//                               Will also force the port of the resulting
//                               socket address to be 77.
//    "MAP * baz, EXCLUDE www.google.com" --> Remaps everything to "baz",
//                                            except for "www.google.com".
//
// These mappings apply to the endpoint host in a net::URLRequest (the TCP
// connect and host resolver in a direct connection, and the CONNECT in an http
// proxy connection, and the endpoint host in a SOCKS proxy connection).
const char kHostRules[] = "host-rules";

// Don't use a proxy server, always make direct connections. Overrides any
// other proxy server flags that are passed.
const char kNoProxyServer[] = "no-proxy-server";

// Uses a specified proxy server, overrides system settings. This switch only
// affects HTTP and HTTPS requests.
const char kProxyServer[] = "proxy-server";

// Bypass specified proxy for the given semi-colon-separated list of hosts. This
// flag has an effect only when --proxy-server is set.
const char kProxyBypassList[] = "proxy-bypass-list";

// Uses the pac script at the given URL.
const char kProxyPacUrl[] = "proxy-pac-url";

// Disable HTTP/2 and SPDY/3.1 protocols.
const char kDisableHttp2[] = "disable-http2";

// Disable HTTP cache.
const char kDisableHttpCache[] = "disable-http-cache";

// Whitelist containing servers for which Integrated Authentication is enabled.
const char kAuthServerWhitelist[] = "auth-server-whitelist";

// Whitelist containing servers for which Kerberos delegation is allowed.
const char kAuthNegotiateDelegateWhitelist[] =
    "auth-negotiate-delegate-whitelist";

// Forces the maximum disk space to be used by the disk cache, in bytes.
const char kDiskCacheSize[] = "disk-cache-size";

}  // namespace switches

}  // namespace atom
