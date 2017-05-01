// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_CERTIFICATE_TRUST_H_
#define ATOM_BROWSER_UI_CERTIFICATE_TRUST_H_

#include <string>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "net/cert/x509_certificate.h"

namespace atom {
class NativeWindow;
}  // namespace atom

namespace certificate_trust {

typedef base::Callback<void(void)> ShowTrustCallback;

void ShowCertificateTrust(atom::NativeWindow* parent_window,
                          const scoped_refptr<net::X509Certificate>& cert,
                          const std::string& message,
                          const ShowTrustCallback& callback);

}  // namespace certificate_trust

#endif  // ATOM_BROWSER_UI_CERTIFICATE_TRUST_H_
