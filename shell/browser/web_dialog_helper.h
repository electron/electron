// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_WEB_DIALOG_HELPER_H_
#define SHELL_BROWSER_WEB_DIALOG_HELPER_H_

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "net/base/directory_lister.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"

namespace base {
class FilePath;
}

namespace content {
class FileSelectListener;
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace electron {

class DirectoryListerHelperDelegate {
 public:
  virtual void OnDirectoryListerDone(
      std::vector<blink::mojom::FileChooserFileInfoPtr> file_info,
      base::FilePath base_dir) = 0;
};

class DirectoryListerHelper
    : public net::DirectoryLister::DirectoryListerDelegate {
 public:
  DirectoryListerHelper(base::FilePath base,
                        DirectoryListerHelperDelegate* helper);
  ~DirectoryListerHelper() override;

 private:
  void OnListFile(
      const net::DirectoryLister::DirectoryListerData& data) override;
  void OnListDone(int error) override;

  base::FilePath base_dir_;
  DirectoryListerHelperDelegate* delegate_;
  std::vector<base::FilePath> paths_;

  DISALLOW_COPY_AND_ASSIGN(DirectoryListerHelper);
};

class NativeWindow;

class WebDialogHelper {
 public:
  WebDialogHelper(NativeWindow* window, bool offscreen);
  ~WebDialogHelper();

  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      std::unique_ptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params);
  void EnumerateDirectory(content::WebContents* web_contents,
                          std::unique_ptr<content::FileSelectListener> listener,
                          const base::FilePath& path);

 private:
  NativeWindow* window_;
  bool offscreen_;

  base::WeakPtrFactory<WebDialogHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebDialogHelper);
};

}  // namespace electron

#endif  // SHELL_BROWSER_WEB_DIALOG_HELPER_H_
