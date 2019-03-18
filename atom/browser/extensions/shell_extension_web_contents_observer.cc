// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_extension_web_contents_observer.h"

namespace extensions {

ShellExtensionWebContentsObserver::ShellExtensionWebContentsObserver(
    content::WebContents* web_contents)
    : ExtensionWebContentsObserver(web_contents) {}

ShellExtensionWebContentsObserver::~ShellExtensionWebContentsObserver() {}

void ShellExtensionWebContentsObserver::CreateForWebContents(
    content::WebContents* web_contents) {
  content::WebContentsUserData<
      ShellExtensionWebContentsObserver>::CreateForWebContents(web_contents);

  // Initialize this instance if necessary.
  FromWebContents(web_contents)->Initialize();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ShellExtensionWebContentsObserver)

}  // namespace extensions
