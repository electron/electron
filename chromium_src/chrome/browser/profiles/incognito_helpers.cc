// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/incognito_helpers.h"

#include "brave/browser/brave_browser_context.h"

namespace chrome {

content::BrowserContext* GetBrowserContextRedirectedInIncognito(
    content::BrowserContext* context) {
  return static_cast<brave::BraveBrowserContext*>(context)->original_context();
}

content::BrowserContext* GetBrowserContextOwnInstanceInIncognito(
    content::BrowserContext* context) {
  return context;
}

}  // namespace chrome
