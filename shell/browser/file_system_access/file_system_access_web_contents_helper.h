// Copyright (c) 2024 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_FILE_SYSTEM_ACCESS_FILE_SYSTEM_ACCESS_WEB_CONTENTS_HELPER_H_
#define ELECTRON_SHELL_BROWSER_FILE_SYSTEM_ACCESS_FILE_SYSTEM_ACCESS_WEB_CONTENTS_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class FileSystemAccessWebContentsHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<FileSystemAccessWebContentsHelper> {
 public:
  FileSystemAccessWebContentsHelper(const FileSystemAccessWebContentsHelper&) =
      delete;
  FileSystemAccessWebContentsHelper& operator=(
      const FileSystemAccessWebContentsHelper&) = delete;
  ~FileSystemAccessWebContentsHelper() override;

  // content::WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;

 private:
  explicit FileSystemAccessWebContentsHelper(
      content::WebContents* web_contents);
  friend class content::WebContentsUserData<FileSystemAccessWebContentsHelper>;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // ELECTRON_SHELL_BROWSER_FILE_SYSTEM_ACCESS_FILE_SYSTEM_ACCESS_WEB_CONTENTS_HELPER_H_
