// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/download_manager_delegate.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/common/gin_helper/dictionary.h"

#include "ui/shell_dialogs/execute_select_file_win.h"

namespace content {
class DownloadManager;
}

namespace electron {

class ElectronDownloadManagerDelegate
    : public content::DownloadManagerDelegate {
 public:
  using CreateDownloadPathCallback =
      base::RepeatingCallback<void(const base::FilePath&)>;

  explicit ElectronDownloadManagerDelegate(content::DownloadManager* manager);
  ~ElectronDownloadManagerDelegate() override;

  // disable copy
  ElectronDownloadManagerDelegate(const ElectronDownloadManagerDelegate&) =
      delete;
  ElectronDownloadManagerDelegate& operator=(
      const ElectronDownloadManagerDelegate&) = delete;

  // content::DownloadManagerDelegate:
  void Shutdown() override;
  bool DetermineDownloadTarget(
      download::DownloadItem* download,
      content::DownloadTargetCallback* callback) override;
  bool ShouldOpenDownload(
      download::DownloadItem* download,
      content::DownloadOpenDelayedCallback callback) override;
  void GetNextId(content::DownloadIdCallback callback) override;

 private:
  // Get the save path set on the associated api::DownloadItem object
  void GetItemSavePath(download::DownloadItem* item, base::FilePath* path);
  void GetItemSaveDialogOptions(download::DownloadItem* item,
                                file_dialog::DialogSettings* options);

  void OnDownloadPathGenerated(uint32_t download_id,
                               content::DownloadTargetCallback callback,
                               const base::FilePath& default_path);

  void OnDownloadSaveDialogDone(
      uint32_t download_id,
      content::DownloadTargetCallback download_callback,
      gin_helper::Dictionary result);

  base::FilePath last_saved_directory_;

  content::DownloadManager* download_manager_;
  base::WeakPtrFactory<ElectronDownloadManagerDelegate> weak_ptr_factory_{this};

  // Copied from ui/shell_dialogs/select_file_dialog_win.h
  bool GetRegistryDescriptionFromExtension(const std::u16string& file_ext,
                                           std::u16string* reg_description);
  std::vector<ui::FileFilterSpec> FormatFilterForExtensions(
      const std::vector<std::u16string>& file_ext,
      const std::vector<std::u16string>& ext_desc,
      bool include_all_files,
      bool keep_extension_visible);
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_
