// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_SAVE_PAGE_HANDLER_H_
#define ATOM_BROWSER_API_SAVE_PAGE_HANDLER_H_

#include <string>

#include "components/download/public/common/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/save_page_type.h"
#include "v8/include/v8.h"

namespace base {
class FilePath;
}

namespace content {
class WebContents;
}

namespace atom {

namespace api {

// A self-destroyed class for handling save page request.
class SavePageHandler : public content::DownloadManager::Observer,
                        public download::DownloadItem::Observer {
 public:
  using SavePageCallback = base::Callback<void(v8::Local<v8::Value>)>;

  SavePageHandler(content::WebContents* web_contents,
                  const SavePageCallback& callback);
  ~SavePageHandler() override;

  bool Handle(const base::FilePath& full_path,
              const content::SavePageType& save_type);

 private:
  void Destroy(download::DownloadItem* item);

  // content::DownloadManager::Observer:
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;

  // download::DownloadItem::Observer:
  void OnDownloadUpdated(download::DownloadItem* item) override;

  content::WebContents* web_contents_;  // weak
  SavePageCallback callback_;
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_SAVE_PAGE_HANDLER_H_
