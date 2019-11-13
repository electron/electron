// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_DOWNLOAD_ITEM_H_
#define SHELL_BROWSER_API_ATOM_API_DOWNLOAD_ITEM_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "components/download/public/common/download_item.h"
#include "gin/handle.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/common/gin_helper/trackable_object.h"
#include "url/gurl.h"

namespace electron {

namespace api {

class DownloadItem : public gin_helper::TrackableObject<DownloadItem>,
                     public download::DownloadItem::Observer {
 public:
  static gin::Handle<DownloadItem> Create(v8::Isolate* isolate,
                                          download::DownloadItem* item);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void Pause();
  bool IsPaused() const;
  void Resume();
  bool CanResume() const;
  void Cancel();
  int64_t GetReceivedBytes() const;
  int64_t GetTotalBytes() const;
  std::string GetMimeType() const;
  bool HasUserGesture() const;
  std::string GetFilename() const;
  std::string GetContentDisposition() const;
  const GURL& GetURL() const;
  const std::vector<GURL>& GetURLChain() const;
  download::DownloadItem::DownloadState GetState() const;
  bool IsDone() const;
  void SetSavePath(const base::FilePath& path);
  base::FilePath GetSavePath() const;
  file_dialog::DialogSettings GetSaveDialogOptions() const;
  void SetSaveDialogOptions(const file_dialog::DialogSettings& options);
  std::string GetLastModifiedTime() const;
  std::string GetETag() const;
  double GetStartTime() const;

 protected:
  DownloadItem(v8::Isolate* isolate, download::DownloadItem* download_item);
  ~DownloadItem() override;

  // Override download::DownloadItem::Observer methods
  void OnDownloadUpdated(download::DownloadItem* download) override;
  void OnDownloadDestroyed(download::DownloadItem* download) override;

 private:
  base::FilePath save_path_;
  file_dialog::DialogSettings dialog_options_;
  download::DownloadItem* download_item_;

  DISALLOW_COPY_AND_ASSIGN(DownloadItem);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_DOWNLOAD_ITEM_H_
