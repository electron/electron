// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_WEBAUTHN_CHROME_AUTHENTICATOR_REQUEST_DELEGATE_H_
#define ELECTRON_BROWSER_WEBAUTHN_CHROME_AUTHENTICATOR_REQUEST_DELEGATE_H_

#include "content/public/browser/authenticator_request_client_delegate.h"

namespace electron {
// ElectronWebAuthenticationDelegate is the //Electron layer implementation of
// content::WebAuthenticationDelegate.
// Based on Chrome browser class ChromeWebAuthenticationDelegate
class ElectronWebAuthenticationDelegate
    : public content::WebAuthenticationDelegate {
 public:
  ~ElectronWebAuthenticationDelegate() override;

#if !BUILDFLAG(IS_ANDROID)
  bool SupportsResidentKeys(
      content::RenderFrameHost* render_frame_host) override;
#endif
};

}  // namespace electron
#endif