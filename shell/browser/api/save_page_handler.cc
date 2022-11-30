// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/save_page_handler.h"

#include <utility>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/electron_browser_context.h"

namespace electron::api {

SavePageHandler::SavePageHandler(content::WebContents* web_contents,
                                 gin_helper::Promise<void> promise)
    : web_contents_(web_contents), promise_(std::move(promise)) {}

SavePageHandler::~SavePageHandler() = default;

void SavePageHandler::OnDownloadCreated(content::DownloadManager* manager,
                                        download::DownloadItem* item) {
  // OnDownloadCreated is invoked during WebContents::SavePage, so the |item|
  // here is the one stated by WebContents::SavePage.
  item->AddObserver(this);
}

bool SavePageHandler::Handle(const base::FilePath& full_path,
                             const content::SavePageType& save_type) {
  auto* download_manager =
      web_contents_->GetBrowserContext()->GetDownloadManager();
  download_manager->AddObserver(this);
  // Chromium will create a 'foo_files' directory under the directory of saving
  // page 'foo.html' for holding other resource files of 'foo.html'.
  base::FilePath saved_main_directory_path = full_path.DirName().Append(
      full_path.RemoveExtension().BaseName().value() +
      FILE_PATH_LITERAL("_files"));
  bool result =
      web_contents_->SavePage(full_path, saved_main_directory_path, save_type);
  download_manager->RemoveObserver(this);
  // If initialization fails which means fail to create |DownloadItem|, we need
  // to delete the |SavePageHandler| instance to avoid memory-leak.
  if (!result) {
    promise_.RejectWithErrorMessage("Failed to save the page");
    delete this;
  }
  return result;
}

void SavePageHandler::OnDownloadUpdated(download::DownloadItem* item) {
  if (item->IsDone()) {
    if (item->GetState() == download::DownloadItem::COMPLETE)
      promise_.Resolve();
    else
      promise_.RejectWithErrorMessage("Failed to save the page.");
    Destroy(item);
  }
}

void SavePageHandler::Destroy(download::DownloadItem* item) {
  item->RemoveObserver(this);
  delete this;
}

}  // namespace electron::api
