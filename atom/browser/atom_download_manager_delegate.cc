// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_download_manager_delegate.h"

#include <string>

#include "atom/browser/api/atom_api_download_item.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "net/base/filename_util.h"

namespace atom {

AtomDownloadManagerDelegate::AtomDownloadManagerDelegate(
    content::DownloadManager* manager)
    : download_manager_(manager),
      weak_ptr_factory_(this) {}

AtomDownloadManagerDelegate::~AtomDownloadManagerDelegate() {
  if (download_manager_) {
    DCHECK_EQ(static_cast<content::DownloadManagerDelegate*>(this),
              download_manager_->GetDelegate());
    download_manager_->SetDelegate(nullptr);
    download_manager_ = nullptr;
  }
}

void AtomDownloadManagerDelegate::GetItemSavePath(content::DownloadItem* item,
                                                  base::FilePath* path) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  api::DownloadItem* download = api::DownloadItem::FromWrappedClass(isolate,
                                                                    item);
  if (download)
    *path = download->GetSavePath();
}

void AtomDownloadManagerDelegate::CreateDownloadPath(
    const GURL& url,
    const std::string& content_disposition,
    const std::string& suggested_filename,
    const std::string& mime_type,
    const base::FilePath& default_download_path,
    const CreateDownloadPathCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);

  auto generated_name = net::GenerateFileName(url,
                                              content_disposition,
                                              std::string(),
                                              suggested_filename,
                                              mime_type,
                                              "download");

  if (!base::PathExists(default_download_path))
    base::CreateDirectory(default_download_path);

  base::FilePath path(default_download_path.Append(generated_name));
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   base::Bind(callback, path));
}

void AtomDownloadManagerDelegate::OnDownloadPathGenerated(
    uint32_t download_id,
    const content::DownloadTargetCallback& callback,
    const base::FilePath& default_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto item = download_manager_->GetDownload(download_id);
  if (!item)
    return;

  NativeWindow* window = nullptr;
  content::WebContents* web_contents = item->GetWebContents();
  auto relay = web_contents ? NativeWindowRelay::FromWebContents(web_contents)
                            : nullptr;
  if (relay)
    window = relay->window.get();

  base::FilePath path;
  GetItemSavePath(item, &path);
  // Show save dialog if save path was not set already on item
  if (path.empty() && file_dialog::ShowSaveDialog(window, item->GetURL().spec(),
                                                  "", default_path,
                                                  file_dialog::Filters(),
                                                  &path)) {
    // Remember the last selected download directory.
    AtomBrowserContext* browser_context = static_cast<AtomBrowserContext*>(
        download_manager_->GetBrowserContext());
    browser_context->prefs()->SetFilePath(prefs::kDownloadDefaultDirectory,
                                          path.DirName());

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    api::DownloadItem* download_item = api::DownloadItem::FromWrappedClass(
        isolate, item);
    if (download_item)
      download_item->SetSavePath(path);
  }

  // Running the DownloadTargetCallback with an empty FilePath signals that the
  // download should be cancelled.
  // If user cancels the file save dialog, run the callback with empty FilePath.
  callback.Run(path,
               content::DownloadItem::TARGET_DISPOSITION_PROMPT,
               content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, path);
}

void AtomDownloadManagerDelegate::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  download_manager_ = nullptr;
}

bool AtomDownloadManagerDelegate::DetermineDownloadTarget(
    content::DownloadItem* download,
    const content::DownloadTargetCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!download->GetForcedFilePath().empty()) {
    callback.Run(download->GetForcedFilePath(),
                 content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 download->GetForcedFilePath());
    return true;
  }

  // Try to get the save path from JS wrapper.
  base::FilePath save_path;
  GetItemSavePath(download, &save_path);
  if (!save_path.empty()) {
    callback.Run(save_path,
                 content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 save_path);
    return true;
  }

  AtomBrowserContext* browser_context = static_cast<AtomBrowserContext*>(
      download_manager_->GetBrowserContext());
  base::FilePath default_download_path = browser_context->prefs()->GetFilePath(
      prefs::kDownloadDefaultDirectory);

  CreateDownloadPathCallback download_path_callback =
      base::Bind(&AtomDownloadManagerDelegate::OnDownloadPathGenerated,
                 weak_ptr_factory_.GetWeakPtr(),
                 download->GetId(), callback);

  content::BrowserThread::PostTask(
      content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&AtomDownloadManagerDelegate::CreateDownloadPath,
                 weak_ptr_factory_.GetWeakPtr(),
                 download->GetURL(),
                 download->GetContentDisposition(),
                 download->GetSuggestedFilename(),
                 download->GetMimeType(),
                 default_download_path,
                 download_path_callback));
  return true;
}

bool AtomDownloadManagerDelegate::ShouldOpenDownload(
    content::DownloadItem* download,
    const content::DownloadOpenDelayedCallback& callback) {
  return true;
}

void AtomDownloadManagerDelegate::GetNextId(
    const content::DownloadIdCallback& callback) {
  static uint32_t next_id = content::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

}  // namespace atom
