// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_download_manager_delegate.h"

#include <string>
#include <tuple>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/i18n/case_conversion.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/win/registry.h"
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
#include "ui/base/l10n/l10n_util.h"

#define IDS_APP_SAVEAS_ALL_FILES 35936
#define IDS_APP_SAVEAS_EXTENSION_FORMAT 35937

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

// Get the file type description from the registry. This will be "Text Document"
// for .txt files, "JPEG Image" for .jpg files, etc. If the registry doesn't
// have an entry for the file type, we return false, true if the description was
// found. 'file_ext' must be in form ".txt".
// Copied from ui/shell_dialogs/select_file_dialog_win.cc
bool ElectronDownloadManagerDelegate::GetRegistryDescriptionFromExtension(
    const std::u16string& file_ext,
    std::u16string* reg_description) {
  DCHECK(reg_description);
  base::win::RegKey reg_ext(HKEY_CLASSES_ROOT, base::as_wcstr(file_ext),
                            KEY_READ);
  std::wstring reg_app;
  if (reg_ext.ReadValue(nullptr, &reg_app) == ERROR_SUCCESS &&
      !reg_app.empty()) {
    base::win::RegKey reg_link(HKEY_CLASSES_ROOT, reg_app.c_str(), KEY_READ);
    std::wstring description;
    if (reg_link.ReadValue(nullptr, &description) == ERROR_SUCCESS) {
      *reg_description = base::WideToUTF16(description);
      return true;
    }
  }
  return false;
}

// Set up a filter for a Save/Open dialog, |ext_desc| as the text descriptions
// of the |file_ext| types (optional), and (optionally) the default 'All Files'
// view. The purpose of the filter is to show only files of a particular type in
// a Windows Save/Open dialog box. The resulting filter is returned. The filter
// created here are:
//   1. only files that have 'file_ext' as their extension
//   2. all files (only added if 'include_all_files' is true)
// If a description is not provided for a file extension, it will be retrieved
// from the registry. If the file extension does not exist in the registry, a
// default description will be created (e.g. "qqq" yields "QQQ File").
// Copied from ui/shell_dialogs/select_file_dialog_win.cc
std::vector<ui::FileFilterSpec>
ElectronDownloadManagerDelegate::FormatFilterForExtensions(
    const std::vector<std::u16string>& file_ext,
    const std::vector<std::u16string>& ext_desc,
    bool include_all_files,
    bool keep_extension_visible) {
  const std::u16string all_ext = u"*.*";
  const std::u16string all_desc =
      l10n_util::GetStringUTF16(IDS_APP_SAVEAS_ALL_FILES);

  DCHECK(file_ext.size() >= ext_desc.size());

  if (file_ext.empty())
    include_all_files = true;

  std::vector<ui::FileFilterSpec> result;
  result.reserve(file_ext.size() + 1);

  for (size_t i = 0; i < file_ext.size(); ++i) {
    std::u16string ext = file_ext[i];
    std::u16string desc;
    if (i < ext_desc.size())
      desc = ext_desc[i];

    if (ext.empty()) {
      // Force something reasonable to appear in the dialog box if there is no
      // extension provided.
      include_all_files = true;
      continue;
    }

    if (desc.empty()) {
      DCHECK(ext.find(u'.') != std::u16string::npos);
      std::u16string first_extension = ext.substr(ext.find(u'.'));
      size_t first_separator_index = first_extension.find(u';');
      if (first_separator_index != std::u16string::npos)
        first_extension = first_extension.substr(0, first_separator_index);

      // Find the extension name without the preceeding '.' character.
      std::u16string ext_name = first_extension;
      size_t ext_index = ext_name.find_first_not_of(u'.');
      if (ext_index != std::u16string::npos)
        ext_name = ext_name.substr(ext_index);

      if (!ElectronDownloadManagerDelegate::GetRegistryDescriptionFromExtension(
              first_extension, &desc)) {
        // The extension doesn't exist in the registry. Create a description
        // based on the unknown extension type (i.e. if the extension is .qqq,
        // then we create a description "QQQ File").
        desc = l10n_util::GetStringFUTF16(IDS_APP_SAVEAS_EXTENSION_FORMAT,
                                          base::i18n::ToUpper(ext_name));
        include_all_files = true;
      }
      if (desc.empty())
        desc = u"*." + ext_name;
    } else if (keep_extension_visible) {
      // Having '*' in the description could cause the windows file dialog to
      // not include the file extension in the file dialog. So strip out any '*'
      // characters if `keep_extension_visible` is set.
      base::ReplaceChars(desc, u"*", base::StringPiece16(), &desc);
    }

    result.push_back({desc, ext});
  }

  if (include_all_files)
    result.push_back({all_desc, all_ext});

  return result;
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

    auto extension = settings.default_path.FinalExtension();
    if (!extension.empty() && settings.filters.empty()) {
      extension.erase(extension.begin());

      std::u16string ext(extension.begin(), extension.end());
      const std::vector<std::u16string> file_ext{u"." + ext};
      std::vector<ui::FileFilterSpec> filter_spec =
          FormatFilterForExtensions(file_ext, {u""}, true, true);

      std::string extension_spec_conv(filter_spec[0].extension_spec.begin(),
                                      filter_spec[0].extension_spec.end());
      const std::vector<std::string> filter{extension_spec_conv};
      std::string description_conv(filter_spec[0].description.begin(),
                                   filter_spec[0].description.end());

      settings.filters.emplace_back(std::make_pair(description_conv, filter));

      std::vector<std::string> all_files{"*.*"};
      settings.filters.emplace_back(std::make_pair("All Files", all_files));
    }

    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    gin_helper::Promise<gin_helper::Dictionary> dialog_promise(isolate);
    auto dialog_callback = base::BindOnce(
        &ElectronDownloadManagerDelegate::OnDownloadSaveDialogDone,
        base::Unretained(this), download_id, std::move(callback));

    std::ignore = dialog_promise.Then(std::move(dialog_callback));
    file_dialog::ShowSaveDialog(settings, std::move(dialog_promise));
  } else {
    std::move(callback).Run(
        path, download::DownloadItem::TARGET_DISPOSITION_PROMPT,
        download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
        item->GetMixedContentStatus(), path, base::FilePath(),
        std::string() /*mime_type*/, absl::nullopt /*download_schedule*/,
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
           item->GetMixedContentStatus(), path, base::FilePath(),
           std::string() /*mime_type*/, absl::nullopt /*download_schedule*/,
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
        download->GetForcedFilePath(), base::FilePath(),
        std::string() /*mime_type*/, absl::nullopt /*download_schedule*/,
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
        base::FilePath(), std::string() /*mime_type*/,
        absl::nullopt /*download_schedule*/,
        download::DOWNLOAD_INTERRUPT_REASON_NONE);
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
