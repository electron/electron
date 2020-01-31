// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_WEB_CONTENTS_OBSERVER_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_WEB_CONTENTS_OBSERVER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/extension_web_contents_observer.h"

namespace extensions {

// The app_shell version of ExtensionWebContentsObserver.
class ElectronExtensionWebContentsObserver
    : public ExtensionWebContentsObserver,
      public content::WebContentsUserData<
          ElectronExtensionWebContentsObserver> {
 public:
  ~ElectronExtensionWebContentsObserver() override;

  // Creates and initializes an instance of this class for the given
  // |web_contents|, if it doesn't already exist.
  static void CreateForWebContents(content::WebContents* web_contents);

 private:
  friend class content::WebContentsUserData<
      ElectronExtensionWebContentsObserver>;

  explicit ElectronExtensionWebContentsObserver(
      content::WebContents* web_contents);

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionWebContentsObserver);
};

}  // namespace extensions

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_WEB_CONTENTS_OBSERVER_H_
