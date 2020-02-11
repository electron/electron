// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_
#define SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/download_manager_delegate.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/common/gin_helper/dictionary.h"

namespace content {
class DownloadManager;
}

namespace electron {

class ElectronDownloadManagerDelegate
    : public content::DownloadManagerDelegate {
 public:
  using CreateDownloadPathCallback =
      base::Callback<void(const base::FilePath&)>;

  explicit ElectronDownloadManagerDelegate(content::DownloadManager* manager);
  ~ElectronDownloadManagerDelegate() override;

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
                                file_dialog::DialogSettings* settings);

  void OnDownloadPathGenerated(uint32_t download_id,
                               content::DownloadTargetCallback callback,
                               const base::FilePath& default_path);

  void OnDownloadSaveDialogDone(
      uint32_t download_id,
      content::DownloadTargetCallback download_callback,
      gin_helper::Dictionary result);

  content::DownloadManager* download_manager_;
  base::WeakPtrFactory<ElectronDownloadManagerDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ElectronDownloadManagerDelegate);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_
