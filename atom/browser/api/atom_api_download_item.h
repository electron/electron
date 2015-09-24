// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_DOWNLOAD_ITEM_H_
#define ATOM_BROWSER_API_ATOM_API_DOWNLOAD_ITEM_H_

#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/files/file_path.h"
#include "content/public/browser/download_item.h"
#include "native_mate/handle.h"
#include "url/gurl.h"

namespace atom {

namespace api {

class DownloadItem : public mate::EventEmitter,
                     public content::DownloadItem::Observer {
 public:
  class SavePathData : public base::SupportsUserData::Data {
   public:
    explicit SavePathData(const base::FilePath& path);
    const base::FilePath& path();
   private:
    base::FilePath path_;
  };

  static mate::Handle<DownloadItem> Create(v8::Isolate* isolate,
                                           content::DownloadItem* item);
  static void* UserDataKey();

 protected:
  explicit DownloadItem(content::DownloadItem* download_item);
  ~DownloadItem();

  // Override content::DownloadItem::Observer methods
  void OnDownloadUpdated(content::DownloadItem* download) override;
  void OnDownloadDestroyed(content::DownloadItem* download) override;

  void Pause();
  void Resume();
  void Cancel();
  int64 GetReceivedBytes();
  int64 GetTotalBytes();
  std::string GetMimeType();
  bool HasUserGesture();
  std::string GetFilename();
  std::string GetContentDisposition();
  const GURL& GetUrl();
  void SetSavePath(const base::FilePath& path);

 private:
  // mate::Wrappable:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  bool IsDestroyed() const override;

  void Destroy();

  content::DownloadItem* download_item_;

  DISALLOW_COPY_AND_ASSIGN(DownloadItem);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_DOWNLOAD_ITEM_H_
