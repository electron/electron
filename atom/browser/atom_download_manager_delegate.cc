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
#include "base/task/post_task.h"
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

void AtomDownloadManagerDelegate::GetItemSaveDialogOptions(
    download::DownloadItem* item,
    file_dialog::DialogSettings* options) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  api::DownloadItem* download =
      api::DownloadItem::FromWrappedClass(isolate, item);
  if (download)
    *options = download->GetSaveDialogOptions();
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

  // Show save dialog if save path was not set already on item
  base::FilePath path;
  GetItemSavePath(item, &path);
  if (path.empty()) {
    file_dialog::DialogSettings settings;
    GetItemSaveDialogOptions(item, &settings);

    if (!settings.parent_window)
      settings.parent_window = window;
    if (settings.title.size() == 0)
      settings.title = item->GetURL().spec();
    if (!settings.default_path.empty())
      settings.default_path = default_path;

    auto* web_preferences = WebContentsPreferences::From(web_contents);
    const bool offscreen =
        !web_preferences || web_preferences->IsEnabled(options::kOffscreen);
    settings.force_detached = offscreen;

    auto dialog_callback =
        base::Bind(&AtomDownloadManagerDelegate::OnDownloadSaveDialogDone,
                   base::Unretained(this), download_id, callback);
    file_dialog::ShowSaveDialog(settings, dialog_callback);
  } else {
    callback.Run(path, download::DownloadItem::TARGET_DISPOSITION_PROMPT,
                 download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, path,
                 download::DOWNLOAD_INTERRUPT_REASON_NONE);
  }
}

#if defined(MAS_BUILD)
void AtomDownloadManagerDelegate::OnDownloadSaveDialogDone(
    uint32_t download_id,
    const content::DownloadTargetCallback& download_callback,
    bool result,
    const base::FilePath& path,
    const std::string& bookmark)
#else
void AtomDownloadManagerDelegate::OnDownloadSaveDialogDone(
    uint32_t download_id,
    const content::DownloadTargetCallback& download_callback,
    bool result,
    const base::FilePath& path)
#endif
{
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* item = download_manager_->GetDownload(download_id);
  if (!item)
    return;

  if (result) {
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
  // download should be cancelled. If user cancels the file save dialog, run
  // the callback with empty FilePath.
  const base::FilePath download_path = result ? path : base::FilePath();
  const auto interrupt_reason =
      download_path.empty() ? download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED
                            : download::DOWNLOAD_INTERRUPT_REASON_NONE;
  download_callback.Run(download_path,
                        download::DownloadItem::TARGET_DISPOSITION_PROMPT,
                        download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                        download_path, interrupt_reason);
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
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
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
