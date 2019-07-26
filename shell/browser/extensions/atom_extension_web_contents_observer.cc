// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/atom_extension_web_contents_observer.h"

namespace extensions {

AtomExtensionWebContentsObserver::AtomExtensionWebContentsObserver(
    content::WebContents* web_contents)
    : ExtensionWebContentsObserver(web_contents) {}

AtomExtensionWebContentsObserver::~AtomExtensionWebContentsObserver() {}

void AtomExtensionWebContentsObserver::CreateForWebContents(
    content::WebContents* web_contents) {
  content::WebContentsUserData<
      AtomExtensionWebContentsObserver>::CreateForWebContents(web_contents);

  // Initialize this instance if necessary.
  FromWebContents(web_contents)->Initialize();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AtomExtensionWebContentsObserver)

}  // namespace extensions
