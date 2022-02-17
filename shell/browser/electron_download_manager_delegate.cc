// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_download_manager_delegate.h"

#include <string>
#include <tuple>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/common/pref_names.h"
#include "components/download/public/common/download_danger_type.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/download_manager.h"
#include "net/base/filename_util.h"
#include "shell/browser/api/electron_api_download_item.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/options_switches.h"

namespace electron {

namespace {

// Generate default file path to save the download.
base::FilePath CreateDownloadPath(const GURL& url,
                                  const std::string& content_disposition,
                                  const std::string& suggested_filename,
                                  const std::string& mime_type,
                                  const base::FilePath& last_saved_directory,
                                  const base::FilePath& default_download_path) {
  auto generated_name =
      net::GenerateFileName(url, content_disposition, std::string(),
                            suggested_filename, mime_type, "download");

  base::FilePath download_path;

  // If the last saved directory is a non-empty existent path, use it as the
  // default.
  if (last_saved_directory.empty() || !base::PathExists(last_saved_directory)) {
    download_path = default_download_path;

    if (!base::PathExists(download_path))
      base::CreateDirectory(download_path);
  } else {
    // Otherwise use the global default.
    download_path = last_saved_directory;
  }

  return download_path.Append(generated_name);
}

}  // namespace

ElectronDownloadManagerDelegate::ElectronDownloadManagerDelegate(
    content::DownloadManager* manager)
    : download_manager_(manager) {}

ElectronDownloadManagerDelegate::~ElectronDownloadManagerDelegate() {
  if (download_manager_) {
    DCHECK_EQ(static_cast<content::DownloadManagerDelegate*>(this),
              download_manager_->GetDelegate());
    download_manager_->SetDelegate(nullptr);
    download_manager_ = nullptr;
  }
}

void ElectronDownloadManagerDelegate::GetItemSavePath(
    download::DownloadItem* item,
    base::FilePath* path) {
  api::DownloadItem* download = api::DownloadItem::FromDownloadItem(item);
  if (download)
    *path = download->GetSavePath();
}

void ElectronDownloadManagerDelegate::GetItemSaveDialogOptions(
    download::DownloadItem* item,
    file_dialog::DialogSettings* options) {
  api::DownloadItem* download = api::DownloadItem::FromDownloadItem(item);
  if (download)
    *options = download->GetSaveDialogOptions();
}

void ElectronDownloadManagerDelegate::OnDownloadPathGenerated(
    uint32_t download_id,
    content::DownloadTargetCallback callback,
    const base::FilePath& default_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  base::ThreadRestrictions::ScopedAllowIO allow_io;

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

  // Show save dialog if save path was not set already on item
  base::FilePath path;
  GetItemSavePath(item, &path);
  if (path.empty()) {
    file_dialog::DialogSettings settings;
    GetItemSaveDialogOptions(item, &settings);

    if (!settings.parent_window)
      settings.parent_window = window;
    if (settings.title.empty())
      settings.title = item->GetURL().spec();
    if (settings.default_path.empty())
      settings.default_path = default_path;

    auto* web_preferences = WebContentsPreferences::From(web_contents);
    const bool offscreen = !web_preferences || web_preferences->IsOffscreen();
    settings.force_detached = offscreen;

    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    gin_helper::Promise<gin_helper::Dictionary> dialog_promise(isolate);
    auto dialog_callback = base::BindOnce(
        &ElectronDownloadManagerDelegate::OnDownloadSaveDialogDone,
        base::Unretained(this), download_id, std::move(callback));

    std::ignore = dialog_promise.Then(std::move(dialog_callback));
    file_dialog::ShowSaveDialog(settings, std::move(dialog_promise));
  } else {
    std::move(callback).Run(path,
                            download::DownloadItem::TARGET_DISPOSITION_PROMPT,
                            download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                            item->GetMixedContentStatus(), path, absl::nullopt,
                            download::DOWNLOAD_INTERRUPT_REASON_NONE);
  }
}

void ElectronDownloadManagerDelegate::OnDownloadSaveDialogDone(
    uint32_t download_id,
    content::DownloadTargetCallback download_callback,
    gin_helper::Dictionary result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* item = download_manager_->GetDownload(download_id);
  if (!item)
    return;

  bool canceled = true;
  result.Get("canceled", &canceled);

  base::FilePath path;

  if (!canceled) {
    if (result.Get("filePath", &path)) {
      // Remember the last selected download directory.
      last_saved_directory_ = path.DirName();

      api::DownloadItem* download = api::DownloadItem::FromDownloadItem(item);
      if (download)
        download->SetSavePath(path);
    }
  }

  // Running the DownloadTargetCallback with an empty FilePath signals that the
  // download should be cancelled. If user cancels the file save dialog, run
  // the callback with empty FilePath.
  const auto interrupt_reason =
      path.empty() ? download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED
                   : download::DOWNLOAD_INTERRUPT_REASON_NONE;
  std::move(download_callback)
      .Run(path, download::DownloadItem::TARGET_DISPOSITION_PROMPT,
           download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
           item->GetMixedContentStatus(), path, absl::nullopt,
           interrupt_reason);
}

void ElectronDownloadManagerDelegate::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  download_manager_ = nullptr;
}

bool ElectronDownloadManagerDelegate::DetermineDownloadTarget(
    download::DownloadItem* download,
    content::DownloadTargetCallback* callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!download->GetForcedFilePath().empty()) {
    std::move(*callback).Run(
        download->GetForcedFilePath(),
        download::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
        download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
        download::DownloadItem::MixedContentStatus::UNKNOWN,
        download->GetForcedFilePath(), absl::nullopt,
        download::DOWNLOAD_INTERRUPT_REASON_NONE);
    return true;
  }

  // Try to get the save path from JS wrapper.
  base::FilePath save_path;
  GetItemSavePath(download, &save_path);
  if (!save_path.empty()) {
    std::move(*callback).Run(
        save_path, download::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
        download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
        download::DownloadItem::MixedContentStatus::UNKNOWN, save_path,
        absl::nullopt, download::DOWNLOAD_INTERRUPT_REASON_NONE);
    return true;
  }

  auto* browser_context = static_cast<ElectronBrowserContext*>(
      download_manager_->GetBrowserContext());
  base::FilePath default_download_path =
      browser_context->prefs()->GetFilePath(prefs::kDownloadDefaultDirectory);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&CreateDownloadPath, download->GetURL(),
                     download->GetContentDisposition(),
                     download->GetSuggestedFilename(), download->GetMimeType(),
                     last_saved_directory_, default_download_path),
      base::BindOnce(&ElectronDownloadManagerDelegate::OnDownloadPathGenerated,
                     weak_ptr_factory_.GetWeakPtr(), download->GetId(),
                     std::move(*callback)));
  return true;
}

bool ElectronDownloadManagerDelegate::ShouldOpenDownload(
    download::DownloadItem* download,
    content::DownloadOpenDelayedCallback callback) {
  return true;
}

void ElectronDownloadManagerDelegate::GetNextId(
    content::DownloadIdCallback callback) {
  static uint32_t next_id = download::DownloadItem::kInvalidId + 1;
  std::move(callback).Run(next_id++);
}

}  // namespace electron
