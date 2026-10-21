// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_RELAY_H_
#define ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_RELAY_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_user_data.h"

namespace electron {

class NativeWindow;

// This class provides a hook to get a NativeWindow from a WebContents.
class NativeWindowRelay
    : public content::WebContentsUserData<NativeWindowRelay> {
 public:
  static void CreateForWebContents(content::WebContents*,
                                   base::WeakPtr<NativeWindow>);

  ~NativeWindowRelay() override;

  NativeWindow* GetNativeWindow() const { return native_window_.get(); }

  WEB_CONTENTS_USER_DATA_KEY_DECL();

 private:
  friend class content::WebContentsUserData<NativeWindow>;
  explicit NativeWindowRelay(content::WebContents* web_contents,
                             base::WeakPtr<NativeWindow> window);

  base::WeakPtr<NativeWindow> native_window_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NATIVE_WINDOW_RELAY_H_
