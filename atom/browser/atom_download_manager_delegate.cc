// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_download_manager_delegate.h"

#include <string>

#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "base/bind.h"
#include "base/files/file_util.h"
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
                                              std::string());

  if (!base::PathExists(default_download_path))
    base::CreateDirectory(default_download_path);

  base::FilePath path(default_download_path.Append(generated_name));
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   base::Bind(callback, path));
}

void AtomDownloadManagerDelegate::OnDownloadPathGenerated(
    uint32 download_id,
    const content::DownloadTargetCallback& callback,
    const base::FilePath& default_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto item = download_manager_->GetDownload(download_id);
  if (!item)
    return;

  file_dialog::Filters filters;
  base::FilePath path;
  auto owner_window_ = NativeWindow::FromWebContents(item->GetWebContents());
  if (!file_dialog::ShowSaveDialog(
          owner_window_, item->GetURL().spec(),
          default_path, filters, &path)) {
    return;
  }

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

  if (default_download_path_.empty()) {
    auto path = download_manager_->GetBrowserContext()->GetPath();
    default_download_path_ = path.Append(FILE_PATH_LITERAL("Downloads"));
  }

  if (!download->GetForcedFilePath().empty()) {
    callback.Run(download->GetForcedFilePath(),
                 content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 download->GetForcedFilePath());
    return true;
  }

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
                 default_download_path_,
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
  static uint32 next_id = content::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

}  // namespace atom
