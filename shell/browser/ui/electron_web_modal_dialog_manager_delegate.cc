#include "shell/browser/ui/electron_web_modal_dialog_manager_delegate.h"

#include "content/public/browser/web_contents.h"
#include "shell/common/platform_util.h"

ElectronWebModalDialogManagerDelegate::ElectronWebModalDialogManagerDelegate() {
}

ElectronWebModalDialogManagerDelegate::
    ~ElectronWebModalDialogManagerDelegate() {}

bool ElectronWebModalDialogManagerDelegate::IsWebContentsVisible(
    content::WebContents* web_contents) {
  return platform_util::IsVisible(web_contents->GetNativeView());
}