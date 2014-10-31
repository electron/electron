// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_dialog_helper.h"

#include <vector>

#include "atom/browser/ui/file_dialog.h"
#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

namespace atom {

WebDialogHelper::WebDialogHelper(NativeWindow* window)
    : window_(window),
      weak_factory_(this) {
}

WebDialogHelper::~WebDialogHelper() {
}

void WebDialogHelper::RunFileChooser(content::WebContents* web_contents,
                                     const content::FileChooserParams& params) {
}

void WebDialogHelper::EnumerateDirectory(content::WebContents* web_contents,
                                         int request_id,
                                         const base::FilePath& dir) {
  int types = base::FileEnumerator::FILES |
              base::FileEnumerator::DIRECTORIES |
              base::FileEnumerator::INCLUDE_DOT_DOT;
  base::FileEnumerator file_enum(dir, false, types);

  base::FilePath path;
  std::vector<base::FilePath> paths;
  while (!(path = file_enum.Next()).empty())
    paths.push_back(path);

  web_contents->GetRenderViewHost()->DirectoryEnumerationFinished(
      request_id, paths);
}

}  // namespace atom
