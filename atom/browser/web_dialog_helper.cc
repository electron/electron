// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_dialog_helper.h"

#include "atom/browser/ui/file_dialog.h"
#include "base/bind.h"

namespace atom {

WebDialogHelper::WebDialogHelper(content::WebContents* web_contents,
                                 NativeWindow* window)
    : web_contents_(web_contents),
      window_(window),
      weak_factory_(this) {
}

WebDialogHelper::~WebDialogHelper() {
}

void WebDialogHelper::RunFileChooser(const content::FileChooserParams& params) {
}

void WebDialogHelper::EnumerateDirectory(int request_id,
                                         const base::FilePath& path) {
}

}  // namespace atom
