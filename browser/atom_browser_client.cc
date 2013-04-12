// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_client.h"

#include "browser/atom_browser_main_parts.h"

namespace atom {

AtomBrowserClient::AtomBrowserClient() {
}

AtomBrowserClient::~AtomBrowserClient() {
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  return new AtomBrowserMainParts;
}

}  // namespace atom
