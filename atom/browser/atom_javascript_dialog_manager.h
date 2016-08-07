// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "content/public/browser/javascript_dialog_manager.h"

namespace atom {

class AtomJavaScriptDialogManager : public content::JavaScriptDialogManager {
 public:
  // content::JavaScriptDialogManager implementations.
  void RunJavaScriptDialog(
      content::WebContents* web_contents,
      const GURL& origin_url,
      content::JavaScriptMessageType javascript_message_type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      const DialogClosedCallback& callback,
      bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(
      content::WebContents* web_contents,
      bool is_reload,
      const DialogClosedCallback& callback) override;
  void CancelActiveAndPendingDialogs(
      content::WebContents* web_contents) override {}
  void ResetDialogState(content::WebContents* web_contents) override {};
};

}  // namespace atom
