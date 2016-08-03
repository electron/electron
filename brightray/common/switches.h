// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_COMMON_SWITCHES_H_
#define BRIGHTRAY_COMMON_SWITCHES_H_

namespace brightray {

namespace switches {

extern const char kHostRules[];
extern const char kNoProxyServer[];
extern const char kProxyServer[];
extern const char kProxyBypassList[];
extern const char kProxyPacUrl[];
extern const char kDisableHttp2[];
extern const char kAuthServerWhitelist[];
extern const char kAuthNegotiateDelegateWhitelist[];
extern const char kCookieableSchemes[];

}  // namespace switches

}  // namespace brightray

#endif  // BRIGHTRAY_COMMON_SWITCHES_H_
