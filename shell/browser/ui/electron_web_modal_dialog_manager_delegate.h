#ifndef SHELL_BROWSER_UI_CHROME_WEB_MODAL_DIALOG_MANAGER_DELEGATE_H_
#define SHELL_BROWSER_UI_CHROME_WEB_MODAL_DIALOG_MANAGER_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/web_modal/web_contents_modal_dialog_manager_delegate.h"

class ElectronWebModalDialogManagerDelegate
    : public web_modal::WebContentsModalDialogManagerDelegate {
 public:
  ElectronWebModalDialogManagerDelegate();
  ~ElectronWebModalDialogManagerDelegate() override;

 protected:
  // Overridden from web_modal::WebContentsModalDialogManagerDelegate:
  bool IsWebContentsVisible(content::WebContents* web_contents) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronWebModalDialogManagerDelegate);
};

#endif  // SHELL_BROWSER_UI_CHROME_WEB_MODAL_DIALOG_MANAGER_DELEGATE_H_