// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_download_manager_delegate.h"

#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
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
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/options_switches.h"
#include "shell/common/thread_restrictions.h"

#if BUILDFLAG(IS_WIN)
#include <vector>

#include "base/i18n/case_conversion.h"
#include "base/win/registry.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/shell_dialogs/execute_select_file_win.h"
#include "ui/strings/grit/ui_strings.h"
#endif  // BUILDFLAG(IS_WIN)

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

#if BUILDFLAG(IS_WIN)
// Get the file type description from the registry. This will be "Text Document"
// for .txt files, "JPEG Image" for .jpg files, etc. If the registry doesn't
// have an entry for the file type, we return false, true if the description was
// found. 'file_ext' must be in form ".txt".
// Modified from ui/shell_dialogs/select_file_dialog_win.cc
bool GetRegistryDescriptionFromExtension(const std::string& file_ext,
                                         std::string* reg_description) {
  DCHECK(reg_description);
  base::win::RegKey reg_ext(HKEY_CLASSES_ROOT,
                            base::UTF8ToWide(file_ext).c_str(), KEY_READ);
  std::wstring reg_app;
  if (reg_ext.ReadValue(nullptr, &reg_app) == ERROR_SUCCESS &&
      !reg_app.empty()) {
    base::win::RegKey reg_link(HKEY_CLASSES_ROOT, reg_app.c_str(), KEY_READ);
    std::wstring description;
    if (reg_link.ReadValue(nullptr, &description) == ERROR_SUCCESS) {
      *reg_description = base::WideToUTF8(description);
      return true;
    }
  }
  return false;
}

// Set up a filter for a Save/Open dialog, |ext_desc| as the text descriptions
// of the |file_ext| types (optional), and (optionally) the default 'All Files'
// view. The purpose of the filter is to show only files of a particular type in
// a Windows Save/Open dialog box. The resulting filter is returned. The filters
// created here are:
//   1. only files that have 'file_ext' as their extension
//   2. all files (only added if 'include_all_files' is true)
// If a description is not provided for a file extension, it will be retrieved
// from the registry. If the file extension does not exist in the registry, a
// default description will be created (e.g. "qqq" yields "QQQ File").
// Modified from ui/shell_dialogs/select_file_dialog_win.cc
file_dialog::Filters FormatFilterForExtensions(
    const std::vector<std::string>& file_ext,
    const std::vector<std::string>& ext_desc,
    bool include_all_files,
    bool keep_extension_visible) {
  const std::string all_ext = "*";
  const std::string all_desc =
      l10n_util::GetStringUTF8(IDS_APP_SAVEAS_ALL_FILES);

  DCHECK(file_ext.size() >= ext_desc.size());

  if (file_ext.empty())
    include_all_files = true;

  file_dialog::Filters result;
  result.reserve(file_ext.size() + 1);

  for (size_t i = 0; i < file_ext.size(); ++i) {
    std::string ext = file_ext[i];
    std::string desc;
    if (i < ext_desc.size())
      desc = ext_desc[i];

    if (ext.empty()) {
      // Force something reasonable to appear in the dialog box if there is no
      // extension provided.
      include_all_files = true;
      continue;
    }

    if (desc.empty()) {
      DCHECK(ext.find('.') != std::string::npos);
      std::string first_extension = ext.substr(ext.find('.'));
      size_t first_separator_index = first_extension.find(';');
      if (first_separator_index != std::string::npos)
        first_extension = first_extension.substr(0, first_separator_index);

      // Find the extension name without the preceding '.' character.
      std::string ext_name = first_extension;
      size_t ext_index = ext_name.find_first_not_of('.');
      if (ext_index != std::string::npos)
        ext_name = ext_name.substr(ext_index);

      if (!GetRegistryDescriptionFromExtension(first_extension, &desc)) {
        // The extension doesn't exist in the registry. Create a description
        // based on the unknown extension type (i.e. if the extension is .qqq,
        // then we create a description "QQQ File").
        desc = l10n_util::GetStringFUTF8(
            IDS_APP_SAVEAS_EXTENSION_FORMAT,
            base::i18n::ToUpper(base::UTF8ToUTF16(ext_name)));
        include_all_files = true;
      }
      if (desc.empty())
        desc = "*." + ext_name;
    } else if (keep_extension_visible) {
      // Having '*' in the description could cause the windows file dialog to
      // not include the file extension in the file dialog. So strip out any '*'
      // characters if `keep_extension_visible` is set.
      base::ReplaceChars(desc, "*", std::string_view(), &desc);
    }

    // Remove the preceding '.' character from the extension.
    size_t ext_index = ext.find_first_not_of('.');
    if (ext_index != std::string::npos)
      ext = ext.substr(ext_index);
    result.push_back({desc, {ext}});
  }

  if (include_all_files)
    result.push_back({all_desc, {all_ext}});

  return result;
}
#endif  // BUILDFLAG(IS_WIN)

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
    download::DownloadTargetCallback callback,
    const base::FilePath& default_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ScopedAllowBlockingForElectron allow_blocking;

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

#if BUILDFLAG(IS_WIN)
    if (settings.filters.empty()) {
      const std::wstring extension = settings.default_path.FinalExtension();
      if (!extension.empty()) {
        settings.filters = FormatFilterForExtensions(
            {base::WideToUTF8(extension)}, {""}, true, true);
      }
    }
#endif  // BUILDFLAG(IS_WIN)

    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    gin_helper::Promise<gin_helper::Dictionary> dialog_promise(isolate);
    auto dialog_callback = base::BindOnce(
        &ElectronDownloadManagerDelegate::OnDownloadSaveDialogDone,
        base::Unretained(this), download_id, std::move(callback));

    std::ignore = dialog_promise.Then(std::move(dialog_callback));
    file_dialog::ShowSaveDialog(settings, std::move(dialog_promise));
  } else {
    download::DownloadTargetInfo target_info;
    target_info.target_path = path;
    target_info.intermediate_path = path;
    target_info.target_disposition =
        download::DownloadItem::TARGET_DISPOSITION_PROMPT;
    target_info.insecure_download_status = item->GetInsecureDownloadStatus();

    std::move(callback).Run(std::move(target_info));
  }
}

void ElectronDownloadManagerDelegate::OnDownloadSaveDialogDone(
    uint32_t download_id,
    download::DownloadTargetCallback download_callback,
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
  download::DownloadTargetInfo target_info;
  target_info.target_path = path;
  target_info.intermediate_path = path;
  target_info.target_disposition =
      download::DownloadItem::TARGET_DISPOSITION_PROMPT;
  target_info.insecure_download_status = item->GetInsecureDownloadStatus();
  target_info.interrupt_reason = interrupt_reason;
  std::move(download_callback).Run(std::move(target_info));
}

void ElectronDownloadManagerDelegate::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  download_manager_ = nullptr;
}

bool ElectronDownloadManagerDelegate::DetermineDownloadTarget(
    download::DownloadItem* download,
    download::DownloadTargetCallback* callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!download->GetForcedFilePath().empty()) {
    download::DownloadTargetInfo target_info;
    target_info.target_path = download->GetForcedFilePath();
    target_info.intermediate_path = download->GetForcedFilePath();
    std::move(*callback).Run(std::move(target_info));
    return true;
  }

  // Try to get the save path from JS wrapper.
  base::FilePath save_path;
  GetItemSavePath(download, &save_path);
  if (!save_path.empty()) {
    download::DownloadTargetInfo target_info;
    target_info.target_path = save_path;
    target_info.intermediate_path = save_path;
    std::move(*callback).Run(std::move(target_info));
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
