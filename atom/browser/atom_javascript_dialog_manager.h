// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_JAVASCRIPT_DIALOG_MANAGER_H_
#define ATOM_BROWSER_ATOM_JAVASCRIPT_DIALOG_MANAGER_H_

#include <string>

#include "content/public/browser/javascript_dialog_manager.h"

namespace atom {

class AtomJavaScriptDialogManager : public content::JavaScriptDialogManager {
 public:
  // content::JavaScriptDialogManager implementations.
  virtual void RunJavaScriptDialog(
      content::WebContents* web_contents,
      const GURL& origin_url,
      const std::string& accept_lang,
      content::JavaScriptMessageType javascript_message_type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      const DialogClosedCallback& callback,
      bool* did_suppress_message) OVERRIDE;
  virtual void RunBeforeUnloadDialog(
      content::WebContents* web_contents,
      const base::string16& message_text,
      bool is_reload,
      const DialogClosedCallback& callback) OVERRIDE;
  virtual void CancelActiveAndPendingDialogs(
      content::WebContents* web_contents) OVERRIDE {}
  virtual void WebContentsDestroyed(
      content::WebContents* web_contents) OVERRIDE {}
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_JAVASCRIPT_DIALOG_MANAGER_H_
