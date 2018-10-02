// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_download_manager_delegate.h"

#include <string>

#include "atom/browser/api/atom_api_download_item.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/common/options_switches.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/common/pref_names.h"
#include "components/download/public/common/download_danger_type.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/download_manager.h"
#include "net/base/filename_util.h"

namespace atom {

namespace {

// Generate default file path to save the download.
base::FilePath CreateDownloadPath(const GURL& url,
                                  const std::string& content_disposition,
                                  const std::string& suggested_filename,
                                  const std::string& mime_type,
                                  const base::FilePath& default_download_path) {
  auto generated_name =
      net::GenerateFileName(url, content_disposition, std::string(),
                            suggested_filename, mime_type, "download");

  if (!base::PathExists(default_download_path))
    base::CreateDirectory(default_download_path);

  return default_download_path.Append(generated_name);
}

}  // namespace

AtomDownloadManagerDelegate::AtomDownloadManagerDelegate(
    content::DownloadManager* manager)
    : download_manager_(manager), weak_ptr_factory_(this) {}

AtomDownloadManagerDelegate::~AtomDownloadManagerDelegate() {
  if (download_manager_) {
    DCHECK_EQ(static_cast<content::DownloadManagerDelegate*>(this),
              download_manager_->GetDelegate());
    download_manager_->SetDelegate(nullptr);
    download_manager_ = nullptr;
  }
}

void AtomDownloadManagerDelegate::GetItemSavePath(download::DownloadItem* item,
                                                  base::FilePath* path) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  api::DownloadItem* download =
      api::DownloadItem::FromWrappedClass(isolate, item);
  if (download)
    *path = download->GetSavePath();
}

void AtomDownloadManagerDelegate::OnDownloadPathGenerated(
    uint32_t download_id,
    const content::DownloadTargetCallback& callback,
    const base::FilePath& default_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* item = download_manager_->GetDownload(download_id);
  if (!item)
    return;

  NativeWindow* window = nullptr;
  content::WebContents* web_contents =
      content::DownloadItemUtils::GetWebContents(item);
  auto* relay =
      web_contents ? NativeWindowRelay::FromWebContents(web_contents) : nullptr;
  if (relay)
    window = relay->GetNativeWindow();

  auto* web_preferences = WebContentsPreferences::From(web_contents);
  bool offscreen =
      !web_preferences || web_preferences->IsEnabled(options::kOffscreen);

  base::FilePath path;
  GetItemSavePath(item, &path);
  // Show save dialog if save path was not set already on item
  file_dialog::DialogSettings settings;
  settings.parent_window = window;
  settings.force_detached = offscreen;
  settings.title = item->GetURL().spec();
  settings.default_path = default_path;
  if (path.empty() && file_dialog::ShowSaveDialog(settings, &path)) {
    // Remember the last selected download directory.
    AtomBrowserContext* browser_context = static_cast<AtomBrowserContext*>(
        download_manager_->GetBrowserContext());
    browser_context->prefs()->SetFilePath(prefs::kDownloadDefaultDirectory,
                                          path.DirName());

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    api::DownloadItem* download_item =
        api::DownloadItem::FromWrappedClass(isolate, item);
    if (download_item)
      download_item->SetSavePath(path);
  }

  // Running the DownloadTargetCallback with an empty FilePath signals that the
  // download should be cancelled.
  // If user cancels the file save dialog, run the callback with empty FilePath.
  callback.Run(path, download::DownloadItem::TARGET_DISPOSITION_PROMPT,
               download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, path,
               path.empty() ? download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED
                            : download::DOWNLOAD_INTERRUPT_REASON_NONE);
}

void AtomDownloadManagerDelegate::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  download_manager_ = nullptr;
}

bool AtomDownloadManagerDelegate::DetermineDownloadTarget(
    download::DownloadItem* download,
    const content::DownloadTargetCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!download->GetForcedFilePath().empty()) {
    callback.Run(download->GetForcedFilePath(),
                 download::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 download->GetForcedFilePath(),
                 download::DOWNLOAD_INTERRUPT_REASON_NONE);
    return true;
  }

  // Try to get the save path from JS wrapper.
  base::FilePath save_path;
  GetItemSavePath(download, &save_path);
  if (!save_path.empty()) {
    callback.Run(save_path,
                 download::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, save_path,
                 download::DOWNLOAD_INTERRUPT_REASON_NONE);
    return true;
  }

  AtomBrowserContext* browser_context =
      static_cast<AtomBrowserContext*>(download_manager_->GetBrowserContext());
  base::FilePath default_download_path =
      browser_context->prefs()->GetFilePath(prefs::kDownloadDefaultDirectory);

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&CreateDownloadPath, download->GetURL(),
                     download->GetContentDisposition(),
                     download->GetSuggestedFilename(), download->GetMimeType(),
                     default_download_path),
      base::BindOnce(&AtomDownloadManagerDelegate::OnDownloadPathGenerated,
                     weak_ptr_factory_.GetWeakPtr(), download->GetId(),
                     callback));
  return true;
}

bool AtomDownloadManagerDelegate::ShouldOpenDownload(
    download::DownloadItem* download,
    const content::DownloadOpenDelayedCallback& callback) {
  return true;
}

void AtomDownloadManagerDelegate::GetNextId(
    const content::DownloadIdCallback& callback) {
  static uint32_t next_id = download::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

}  // namespace atom
