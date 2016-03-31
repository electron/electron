// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_DIALOG_HELPER_H_
#define ATOM_BROWSER_WEB_DIALOG_HELPER_H_

#include "base/memory/weak_ptr.h"

namespace base {
class FilePath;
}

namespace content {
struct FileChooserParams;
class WebContents;
}

namespace atom {

class NativeWindow;

class WebDialogHelper {
 public:
  explicit WebDialogHelper(NativeWindow* window);
  ~WebDialogHelper();

  void RunFileChooser(content::WebContents* web_contents,
                      const content::FileChooserParams& params);
  void EnumerateDirectory(content::WebContents* web_contents,
                          int request_id,
                          const base::FilePath& path);

 private:
  NativeWindow* window_;

  base::WeakPtrFactory<WebDialogHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebDialogHelper);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_DIALOG_HELPER_H_
