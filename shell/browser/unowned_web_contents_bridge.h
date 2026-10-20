// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license.
#ifndef ELECTRON_SHELL_BROWSER_UNOWNED_WEB_CONTENTS_BRIDGE_H_
#define ELECTRON_SHELL_BROWSER_UNOWNED_WEB_CONTENTS_BRIDGE_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"

namespace electron {

// An embedding relationship only. Never takes ownership of either contents.
class UnownedWebContentsBridge : public content::WebContentsObserver {
 public:
  UnownedWebContentsBridge(content::WebContents* inner,
                           base::RepeatingCallback<void(std::string)> detached);
  ~UnownedWebContentsBridge() override;

  bool IsAttached() const;
  std::string Attach(content::RenderFrameHost* owner,
                     const std::string& frame_token,
                     std::string* error);
  bool Detach();

 private:
  void UnownedInnerWebContentsDetached(content::WebContents* inner) override;
  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override;
  void WebContentsDestroyed() override;
  void CompleteDetach();

  base::WeakPtr<content::WebContents> inner_;
  base::WeakPtr<content::WebContents> outer_;
  base::RepeatingCallback<void(std::string)> detached_;
  std::string attachment_id_;
};

}  // namespace electron
#endif
