// Copyright (c) 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_FILE_SELECT_HELPER_H_
#define SHELL_BROWSER_FILE_SELECT_HELPER_H_

#include <memory>

#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_observer.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "gin/dictionary.h"
#include "net/base/directory_lister.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/common/gin_helper/dictionary.h"
#include "ui/shell_dialogs/selected_file_info.h"

using blink::mojom::FileChooserParams;

class FileSelectHelper : public content::WebContentsObserver,
                         public content::RenderWidgetHostObserver,
                         public net::DirectoryLister::DirectoryListerDelegate {
 public:
  FileSelectHelper(content::RenderFrameHost* render_frame_host,
                   scoped_refptr<content::FileSelectListener> listener,
                   FileChooserParams::Mode mode);
  ~FileSelectHelper() override;

  // WebDialogHelper::RunFileChooser

  void ShowOpenDialog(const file_dialog::DialogSettings& settings);

  void ShowSaveDialog(const file_dialog::DialogSettings& settings);

 private:
  // net::DirectoryLister::DirectoryListerDelegate overrides.
  void OnListFile(
      const net::DirectoryLister::DirectoryListerData& data) override;
  void OnListDone(int error) override;

  void DeleteTemporaryFiles();

  void EnumerateDirectory();

  void OnOpenDialogDone(gin_helper::Dictionary result);
  void OnSaveDialogDone(gin_helper::Dictionary result);

  void OnFilesSelected(
      std::vector<blink::mojom::FileChooserFileInfoPtr> file_info,
      base::FilePath base_dir);

  void RunFileChooserEnd();

  void ConvertToFileChooserFileInfoList(
      const std::vector<ui::SelectedFileInfo>& files);

#if defined(OS_MAC)
  // Must be called from a MayBlock() task. Each selected file that is a package
  // will be zipped, and the zip will be passed to the render view host in place
  // of the package.
  void ProcessSelectedFilesMac(const std::vector<ui::SelectedFileInfo>& files);

  // Saves the paths of |zipped_files| for later deletion. Passes |files| to the
  // render view host.
  void ProcessSelectedFilesMacOnUIThread(
      const std::vector<ui::SelectedFileInfo>& files,
      const std::vector<base::FilePath>& zipped_files);

  // Zips the package at |path| into a temporary destination. Returns the
  // temporary destination, if the zip was successful. Otherwise returns an
  // empty path.
  static base::FilePath ZipPackage(const base::FilePath& path);
#endif  // defined(OS_MAC)

  // content::RenderWidgetHostObserver:
  void RenderWidgetHostDestroyed(
      content::RenderWidgetHost* widget_host) override;

  // content::WebContentsObserver:
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* deleted_host) override;
  void WebContentsDestroyed() override;

  content::RenderFrameHost* render_frame_host_;
  content::WebContents* web_contents_;
  scoped_refptr<content::FileSelectListener> listener_;
  FileChooserParams::Mode mode_;

  ScopedObserver<content::RenderWidgetHost, content::RenderWidgetHostObserver>
      observer_{this};

  // Temporary files only used on OSX. This class is responsible for deleting
  // these files when they are no longer needed.
  std::vector<base::FilePath> temporary_files_;

  // DirectoryLister-specific members
  std::unique_ptr<net::DirectoryLister> lister_;
  base::FilePath lister_base_dir_;
  std::vector<base::FilePath> lister_paths_;

  base::WeakPtrFactory<FileSelectHelper> weak_ptr_factory_{this};
};

#endif  // SHELL_BROWSER_FILE_SELECT_HELPER_H_
