// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DOWNLOAD_ITEM_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DOWNLOAD_ITEM_H_

#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "components/download/public/common/download_item.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/common/gin_helper/self_keep_alive.h"

class GURL;

namespace electron::api {

class DownloadItem final : public gin::Wrappable<DownloadItem>,
                           public gin_helper::EventEmitterMixin<DownloadItem>,
                           private download::DownloadItem::Observer {
 public:
  static DownloadItem* FromOrCreate(v8::Isolate* isolate,
                                    download::DownloadItem* item);

  static DownloadItem* FromDownloadItem(download::DownloadItem* item);

  DownloadItem(v8::Isolate* isolate, download::DownloadItem* item);
  ~DownloadItem() override;

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  static const char* GetClassName() { return "DownloadItem"; }
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

  // JS API, but also C++ calls it sometimes
  void SetSavePath(const base::FilePath& path);
  base::FilePath GetSavePath() const;
  file_dialog::DialogSettings GetSaveDialogOptions() const;

  // disable copy
  DownloadItem(const DownloadItem&) = delete;
  DownloadItem& operator=(const DownloadItem&) = delete;

 private:
  bool CheckAlive() const;

  // download::DownloadItem::Observer
  void OnDownloadUpdated(download::DownloadItem* item) override;
  void OnDownloadDestroyed(download::DownloadItem* item) override;

  // JS API
  void Pause();
  bool IsPaused() const;
  void Resume();
  bool CanResume() const;
  void Cancel();
  int64_t GetCurrentBytesPerSecond() const;
  int64_t GetReceivedBytes() const;
  int64_t GetTotalBytes() const;
  int GetPercentComplete() const;
  std::string GetMimeType() const;
  bool HasUserGesture() const;
  std::string GetFilename() const;
  std::string GetContentDisposition() const;
  const GURL& GetURL() const;
  v8::Local<v8::Value> GetURLChain() const;
  download::DownloadItem::DownloadState GetState() const;
  void SetSaveDialogOptions(const file_dialog::DialogSettings& options);
  std::string GetLastModifiedTime() const;
  std::string GetETag() const;
  double GetStartTime() const;
  double GetEndTime() const;

  base::FilePath save_path_;
  file_dialog::DialogSettings dialog_options_;
  raw_ptr<download::DownloadItem> download_item_;

  raw_ptr<v8::Isolate> isolate_;

  gin_helper::SelfKeepAlive<DownloadItem> keep_alive_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DOWNLOAD_ITEM_H_
