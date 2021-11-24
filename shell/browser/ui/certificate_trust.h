// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_CERTIFICATE_TRUST_H_
#define ELECTRON_SHELL_BROWSER_UI_CERTIFICATE_TRUST_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "net/cert/x509_certificate.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/promise.h"

namespace electron {
class NativeWindow;
}

namespace certificate_trust {

v8::Local<v8::Promise> ShowCertificateTrust(
    electron::NativeWindow* parent_window,
    const scoped_refptr<net::X509Certificate>& cert,
    const std::string& message);

}  // namespace certificate_trust

#endif  // ELECTRON_SHELL_BROWSER_UI_CERTIFICATE_TRUST_H_
