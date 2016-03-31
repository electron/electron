// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/save_page_handler.h"

#include <string>

#include "atom/browser/atom_browser_context.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "content/public/browser/web_contents.h"

namespace atom {

namespace api {

SavePageHandler::SavePageHandler(content::WebContents* web_contents,
                                 const SavePageCallback& callback)
    : web_contents_(web_contents),
      callback_(callback) {
}

SavePageHandler::~SavePageHandler() {
}

void SavePageHandler::OnDownloadCreated(content::DownloadManager* manager,
                                        content::DownloadItem* item) {
  // OnDownloadCreated is invoked during WebContents::SavePage, so the |item|
  // here is the one stated by WebContents::SavePage.
  item->AddObserver(this);
}

bool SavePageHandler::Handle(const base::FilePath& full_path,
                             const content::SavePageType& save_type) {
  auto download_manager = content::BrowserContext::GetDownloadManager(
      web_contents_->GetBrowserContext());
  download_manager->AddObserver(this);
  // Chromium will create a 'foo_files' directory under the directory of saving
  // page 'foo.html' for holding other resource files of 'foo.html'.
  base::FilePath saved_main_directory_path = full_path.DirName().Append(
      full_path.RemoveExtension().BaseName().value() +
      FILE_PATH_LITERAL("_files"));
  bool result = web_contents_->SavePage(full_path,
                                        saved_main_directory_path,
                                        save_type);
  download_manager->RemoveObserver(this);
  // If initialization fails which means fail to create |DownloadItem|, we need
  // to delete the |SavePageHandler| instance to avoid memory-leak.
  if (!result)
    delete this;
  return result;
}

void SavePageHandler::OnDownloadUpdated(content::DownloadItem* item) {
  if (item->IsDone()) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    if (item->GetState() == content::DownloadItem::COMPLETE) {
      callback_.Run(v8::Null(isolate));
    } else {
      v8::Local<v8::String> error_message = v8::String::NewFromUtf8(
          isolate, "Fail to save page");
      callback_.Run(v8::Exception::Error(error_message));
    }
    Destroy(item);
  }
}

void SavePageHandler::Destroy(content::DownloadItem* item) {
  item->RemoveObserver(this);
  delete this;
}

// static
bool SavePageHandler::IsSavePageTypes(const std::string& type) {
  return type == "multipart/related" || type == "text/html";
}

}  // namespace api

}  // namespace atom
