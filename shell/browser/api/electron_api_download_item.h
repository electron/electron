// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_DOWNLOAD_ITEM_H_
#define SHELL_BROWSER_API_ELECTRON_API_DOWNLOAD_ITEM_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "components/download/public/common/download_item.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/common/gin_helper/pinnable.h"
#include "url/gurl.h"

namespace electron {

namespace api {

class DownloadItem : public gin::Wrappable<DownloadItem>,
                     public gin_helper::Pinnable<DownloadItem>,
                     public gin_helper::EventEmitterMixin<DownloadItem>,
                     public download::DownloadItem::Observer {
 public:
  static gin::Handle<DownloadItem> FromOrCreate(v8::Isolate* isolate,
                                                download::DownloadItem* item);

  static DownloadItem* FromDownloadItem(download::DownloadItem*);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // JS API, but also C++ calls it sometimes
  void SetSavePath(const base::FilePath& path);
  base::FilePath GetSavePath() const;
  file_dialog::DialogSettings GetSaveDialogOptions() const;

 private:
  DownloadItem(v8::Isolate* isolate, download::DownloadItem* download_item);
  ~DownloadItem() override;

  bool CheckAlive() const;

  // download::DownloadItem::Observer
  void OnDownloadUpdated(download::DownloadItem* download) override;
  void OnDownloadDestroyed(download::DownloadItem* download) override;

  // JS API
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
  v8::Local<v8::Value> GetURLChain(v8::Isolate*) const;
  download::DownloadItem::DownloadState GetState() const;
  bool IsDone() const;
  void SetSaveDialogOptions(const file_dialog::DialogSettings& options);
  std::string GetLastModifiedTime() const;
  std::string GetETag() const;
  double GetStartTime() const;

  base::FilePath save_path_;
  file_dialog::DialogSettings dialog_options_;
  download::DownloadItem* download_item_;

  base::WeakPtrFactory<DownloadItem> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(DownloadItem);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_DOWNLOAD_ITEM_H_
