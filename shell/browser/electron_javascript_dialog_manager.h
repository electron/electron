// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_JAVASCRIPT_DIALOG_MANAGER_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_JAVASCRIPT_DIALOG_MANAGER_H_

#include <map>
#include <string>

#include "content/public/browser/javascript_dialog_manager.h"

namespace content {
class WebContents;
}

namespace electron {

class ElectronJavaScriptDialogManager
    : public content::JavaScriptDialogManager {
 public:
  ElectronJavaScriptDialogManager();
  ~ElectronJavaScriptDialogManager() override;

  // content::JavaScriptDialogManager implementations.
  void RunJavaScriptDialog(content::WebContents* web_contents,
                           content::RenderFrameHost* rfh,
                           content::JavaScriptDialogType dialog_type,
                           const std::u16string& message_text,
                           const std::u16string& default_prompt_text,
                           DialogClosedCallback callback,
                           bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(content::WebContents* web_contents,
                             content::RenderFrameHost* rfh,
                             bool is_reload,
                             DialogClosedCallback callback) override;
  void CancelDialogs(content::WebContents* web_contents,
                     bool reset_state) override;

 private:
  void OnMessageBoxCallback(DialogClosedCallback callback,
                            const std::string& origin,
                            int code,
                            bool checkbox_checked);

  std::map<std::string, int> origin_counts_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_JAVASCRIPT_DIALOG_MANAGER_H_
