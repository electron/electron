// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_JAVASCRIPT_DIALOG_MANAGER_H_
#define ATOM_BROWSER_ATOM_JAVASCRIPT_DIALOG_MANAGER_H_

#include <string>

#include "content/public/browser/javascript_dialog_manager.h"

namespace atom {

namespace api {
class WebContents;
}

class AtomJavaScriptDialogManager : public content::JavaScriptDialogManager {
 public:
  explicit AtomJavaScriptDialogManager(api::WebContents* api_web_contents);

  // content::JavaScriptDialogManager implementations.
  void RunJavaScriptDialog(
      content::WebContents* web_contents,
      const GURL& origin_url,
      content::JavaScriptDialogType dialog_type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      const DialogClosedCallback& callback,
      bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(
      content::WebContents* web_contents,
      bool is_reload,
      const DialogClosedCallback& callback) override;
  void CancelDialogs(content::WebContents* web_contents,
                     bool reset_state) override;

 private:
  static void OnMessageBoxCallback(const DialogClosedCallback& callback,
                                   int code,
                                   bool checkbox_checked);
  api::WebContents* api_web_contents_;
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_JAVASCRIPT_DIALOG_MANAGER_H_
