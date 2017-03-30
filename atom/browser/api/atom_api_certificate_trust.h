// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_CERTIFICATE_TRUST_H_
#define ATOM_BROWSER_API_ATOM_API_CERTIFICATE_TRUST_H_

#include <string>

#include "base/callback_forward.h"

namespace net {
class X509Certificate;
}  // namespace net

namespace atom {

class NativeWindow;

namespace api {

typedef base::Callback<void(bool result)> ShowTrustCallback;

void ShowCertificateTrustUI(atom::NativeWindow* parent_window,
                            const net::X509Certificate& cert,
                            std::string message,
                            const ShowTrustCallback& callback);

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_CERTIFICATE_TRUST_H_
