// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/options_switches.h"

namespace electron {

namespace options {

const char kAllowRunningInsecureContent[] = "allowRunningInsecureContent";

const char kOffscreen[] = "offscreen";

const char kUseSharedTexture[] = "useSharedTexture";

const char kNodeIntegrationInSubFrames[] = "nodeIntegrationInSubFrames";

// Disable window resizing when HTML Fullscreen API is activated.
const char kDisableHtmlFullscreenWindowResize[] =
    "disableHtmlFullscreenWindowResize";

// Enables JavaScript support.
const char kJavaScript[] = "javascript";

// Enables image support.
const char kImages[] = "images";

// Make TextArea elements resizable.
const char kTextAreasAreResizable[] = "textAreasAreResizable";

// Enables WebGL support.
const char kWebGL[] = "webgl";

// Whether dragging and dropping a file or link onto the page causes a
// navigation.
const char kNavigateOnDragDrop[] = "navigateOnDragDrop";

const char kHiddenPage[] = "hiddenPage";

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
const char kSpellcheck[] = "spellcheck";
#endif

const char kEnablePreferredSizeMode[] = "enablePreferredSizeMode";

}  // namespace options

namespace switches {

// Enable chromium sandbox.
const char kEnableSandbox[] = "enable-sandbox";

// Disable HTTP cache.
const char kDisableHttpCache[] = "disable-http-cache";

// The list of standard schemes.
const char kStandardSchemes[] = "standard-schemes";

// Register schemes to handle service worker.
const char kServiceWorkerSchemes[] = "service-worker-schemes";

// Register schemes as secure.
const char kSecureSchemes[] = "secure-schemes";

// Register schemes as bypassing CSP.
const char kBypassCSPSchemes[] = "bypasscsp-schemes";

// Register schemes as support fetch API.
const char kFetchSchemes[] = "fetch-schemes";

// Register schemes as CORS enabled.
const char kCORSSchemes[] = "cors-schemes";

// Register schemes as streaming responses.
const char kStreamingSchemes[] = "streaming-schemes";

// Register schemes as supporting V8 code cache.
const char kCodeCacheSchemes[] = "code-cache-schemes";

// The browser process app model ID
const char kAppUserModelId[] = "app-user-model-id";

// The application path
const char kAppPath[] = "app-path";

// The command line switch versions of the options.
const char kScrollBounce[] = "scroll-bounce";

// Command switch passed to renderer process to control nodeIntegration.
const char kNodeIntegrationInWorker[] = "node-integration-in-worker";

// Widevine options
// Path to Widevine CDM binaries.
const char kWidevineCdmPath[] = "widevine-cdm-path";
// Widevine CDM version.
const char kWidevineCdmVersion[] = "widevine-cdm-version";

// Forces the maximum disk space to be used by the disk cache, in bytes.
const char kDiskCacheSize[] = "disk-cache-size";

// Ignore the limit of 6 connections per host.
const char kIgnoreConnectionsLimit[] = "ignore-connections-limit";

// Whitelist containing servers for which Integrated Authentication is enabled.
const char kAuthServerWhitelist[] = "auth-server-whitelist";

// Whitelist containing servers for which Kerberos delegation is allowed.
const char kAuthNegotiateDelegateWhitelist[] =
    "auth-negotiate-delegate-whitelist";

// If set, include the port in generated Kerberos SPNs.
const char kEnableAuthNegotiatePort[] = "enable-auth-negotiate-port";

// If set, NTLM v2 is disabled for POSIX platforms.
const char kDisableNTLMv2[] = "disable-ntlm-v2";

}  // namespace switches

}  // namespace electron
