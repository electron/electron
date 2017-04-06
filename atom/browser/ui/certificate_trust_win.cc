// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/certificate_trust.h"

#include <windows.h>  // windows.h must be included first

namespace certificate_trust {

void ShowCertificateTrust(atom::NativeWindow* parent_window,
                          const scoped_refptr<net::X509Certificate>& cert,
                          const std::string& message,
                          const ShowTrustCallback& callback) {

}

}  // namespace certificate_trust
