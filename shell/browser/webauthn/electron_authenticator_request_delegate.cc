// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/webauthn/electron_authenticator_request_delegate.h"

namespace electron {
// ---------------------------------------------------------------------
// ElectronWebAuthenticationDelegate
// ---------------------------------------------------------------------

ElectronWebAuthenticationDelegate::~ElectronWebAuthenticationDelegate() =
    default;

#if !BUILDFLAG(IS_ANDROID)
bool ElectronWebAuthenticationDelegate::SupportsResidentKeys(
    content::RenderFrameHost* render_frame_host) {
  return true;
}

#endif  // !IS_ANDROID

}  // namespace electron