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

class DownloadItem : public mate::TrackableObject<DownloadItem>,
                     public content::DownloadItem::Observer {
 public:
  static mate::Handle<DownloadItem> Create(v8::Isolate* isolate,
                                           content::DownloadItem* item);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

  void Pause();
  void Resume();
  void Cancel();
  int64_t GetReceivedBytes() const;
  int64_t GetTotalBytes() const;
  std::string GetMimeType() const;
  bool HasUserGesture() const;
  std::string GetFilename() const;
  std::string GetContentDisposition() const;
  const GURL& GetURL() const;
  content::DownloadItem::DownloadState GetState() const;
  void SetSavePath(const base::FilePath& path);
  base::FilePath GetSavePath() const;

 protected:
  DownloadItem(v8::Isolate* isolate, content::DownloadItem* download_item);
  ~DownloadItem();

  // Override content::DownloadItem::Observer methods
  void OnDownloadUpdated(content::DownloadItem* download) override;
  void OnDownloadDestroyed(content::DownloadItem* download) override;

 private:
  base::FilePath save_path_;
  content::DownloadItem* download_item_;

  DISALLOW_COPY_AND_ASSIGN(DownloadItem);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_DOWNLOAD_ITEM_H_
