// Copyright (c) 2022 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/webauthn/electron_authenticator_request_delegate.h"

namespace electron {

ElectronWebAuthenticationDelegate::~ElectronWebAuthenticationDelegate() =
    default;

bool ElectronWebAuthenticationDelegate::SupportsResidentKeys(
    content::RenderFrameHost* render_frame_host) {
  return true;
}

}  // namespace electron
