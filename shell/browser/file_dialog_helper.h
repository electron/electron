// Copyright (c) 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_FILE_SELECT_HELPER_H_
#define SHELL_BROWSER_FILE_SELECT_HELPER_H_

#include <memory>

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "gin/dictionary.h"
#include "net/base/directory_lister.h"
#include "net/base/mime_util.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "ui/shell_dialogs/selected_file_info.h"

class FileSelectHelper : public base::RefCounted<FileSelectHelper>,
                         public content::WebContentsObserver,
                         public net::DirectoryLister::DirectoryListerDelegate {
 public:
  REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();

  FileSelectHelper(content::RenderFrameHost* render_frame_host,
                   scoped_refptr<content::FileSelectListener> listener,
                   blink::mojom::FileChooserParams::Mode mode);

  void ShowOpenDialog(const file_dialog::DialogSettings& settings);

  void ShowSaveDialog(const file_dialog::DialogSettings& settings);

 private:
  friend class base::RefCounted<FileSelectHelper>;

  ~FileSelectHelper() override;

  // net::DirectoryLister::DirectoryListerDelegate overrides.
  void OnListFile(
      const net::DirectoryLister::DirectoryListerData& data) override;
  void OnListDone(int error) override;

  void EnumerateDirectory();

  void OnOpenDialogDone(gin_helper::Dictionary result);

  void OnSaveDialogDone(gin_helper::Dictionary result);

  void OnFilesSelected(
      std::vector<blink::mojom::FileChooserFileInfoPtr> file_info,
      base::FilePath base_dir);

  void OnSelectionCancelled();

  // content::WebContentsObserver:
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* deleted_host) override;
  void WebContentsDestroyed() override;

  content::RenderFrameHost* render_frame_host_;
  scoped_refptr<content::FileSelectListener> listener_;
  blink::mojom::FileChooserParams::Mode mode_;

  // DirectoryLister-specific members
  std::unique_ptr<net::DirectoryLister> lister_;
  base::FilePath lister_base_dir_;
  std::vector<base::FilePath> lister_paths_;
};

#endif  // SHELL_BROWSER_FILE_SELECT_HELPER_H_