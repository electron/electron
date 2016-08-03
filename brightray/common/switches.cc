// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "common/switches.h"

namespace brightray {

namespace switches {

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

// Whitelist containing servers for which Integrated Authentication is enabled.
const char kAuthServerWhitelist[] = "auth-server-whitelist";

// Whitelist containing servers for which Kerberos delegation is allowed.
const char kAuthNegotiateDelegateWhitelist[] = "auth-negotiate-delegate-whitelist";

// Comma separated list of schemes that should support cookies.
const char kCookieableSchemes[] = "cookieable-schemes";

}  // namespace switches

}  // namespace brightray
