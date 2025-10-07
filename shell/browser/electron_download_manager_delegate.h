// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/download_manager_delegate.h"
#include "shell/browser/ui/file_dialog.h"

namespace content {
class DownloadManager;
}

namespace gin_helper {
class Dictionary;
}

namespace electron {

class ElectronDownloadManagerDelegate
    : public content::DownloadManagerDelegate {
 public:
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
      download::DownloadTargetCallback* callback) override;
  bool ShouldOpenDownload(
      download::DownloadItem* download,
      content::DownloadOpenDelayedCallback callback) override;
  void GetNextId(content::DownloadIdCallback callback) override;

 private:
  // Get the save path set on the associated api::DownloadItem object
  void GetItemSavePath(download::DownloadItem* item, base::FilePath* path);
  void GetItemSaveDialogOptions(download::DownloadItem* item,
                                file_dialog::DialogSettings* options);

  void OnDownloadPathGenerated(const std::string& download_guid,
                               download::DownloadTargetCallback callback,
                               const base::FilePath& default_path);

  void OnDownloadSaveDialogDone(
      const std::string& download_guid,
      download::DownloadTargetCallback download_callback,
      gin_helper::Dictionary result);

  base::FilePath last_saved_directory_;

  raw_ptr<content::DownloadManager> download_manager_;
  base::WeakPtrFactory<ElectronDownloadManagerDelegate> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_
