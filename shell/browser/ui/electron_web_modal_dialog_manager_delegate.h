// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_ELECTRON_WEB_MODAL_DIALOG_MANAGER_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_UI_ELECTRON_WEB_MODAL_DIALOG_MANAGER_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/web_modal/web_contents_modal_dialog_manager_delegate.h"

class ElectronWebModalDialogManagerDelegate
    : public web_modal::WebContentsModalDialogManagerDelegate {
 public:
  ElectronWebModalDialogManagerDelegate();
  ~ElectronWebModalDialogManagerDelegate() override;

  // disable copy
  ElectronWebModalDialogManagerDelegate(
      const ElectronWebModalDialogManagerDelegate&) = delete;
  ElectronWebModalDialogManagerDelegate& operator=(
      const ElectronWebModalDialogManagerDelegate&) = delete;

 protected:
  // Overridden from web_modal::WebContentsModalDialogManagerDelegate:
  bool IsWebContentsVisible(content::WebContents* web_contents) override;
};

#endif  // ELECTRON_SHELL_BROWSER_UI_ELECTRON_WEB_MODAL_DIALOG_MANAGER_DELEGATE_H_
