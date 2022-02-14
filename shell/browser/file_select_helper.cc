// Copyright (c) 2021 Microsoft. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/file_select_helper.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/threading/hang_watcher.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/platform_util.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "electron/grit/electron_resources.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/native_window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/shell_dialogs/selected_file_info.h"

using blink::mojom::FileChooserFileInfo;
using blink::mojom::FileChooserFileInfoPtr;
using blink::mojom::FileChooserParams;
using blink::mojom::FileChooserParamsPtr;
using content::BrowserThread;
using content::RenderViewHost;
using content::RenderWidgetHost;
using content::WebContents;

namespace {

void DeleteFiles(std::vector<base::FilePath> paths) {
  for (auto& file_path : paths)
    base::DeleteFile(file_path);
}

}  // namespace

struct FileSelectHelper::ActiveDirectoryEnumeration {
  explicit ActiveDirectoryEnumeration(const base::FilePath& path)
      : path_(path) {}

  std::unique_ptr<net::DirectoryLister> lister_;
  const base::FilePath path_;
  std::vector<base::FilePath> results_;
};

FileSelectHelper::FileSelectHelper()
    : render_frame_host_(nullptr),
      web_contents_(nullptr),
      select_file_dialog_(),
      select_file_types_(),
      dialog_type_(ui::SelectFileDialog::SELECT_OPEN_FILE),
      dialog_mode_(FileChooserParams::Mode::kOpen) {}

FileSelectHelper::~FileSelectHelper() {
  // There may be pending file dialogs, we need to tell them that we've gone
  // away so they don't try and call back to us.
  if (select_file_dialog_.get())
    select_file_dialog_->ListenerDestroyed();
}

void FileSelectHelper::FileSelected(const base::FilePath& path,
                                    int index,
                                    void* params) {
  FileSelectedWithExtraInfo(ui::SelectedFileInfo(path, path), index, params);
}

void FileSelectHelper::FileSelectedWithExtraInfo(
    const ui::SelectedFileInfo& file,
    int index,
    void* params) {
  if (!render_frame_host_) {
    RunFileChooserEnd();
    return;
  }

  const base::FilePath& path = file.local_path;
  if (dialog_type_ == ui::SelectFileDialog::SELECT_UPLOAD_FOLDER) {
    StartNewEnumeration(path);
    return;
  }

  std::vector<ui::SelectedFileInfo> files;
  files.push_back(file);

#if BUILDFLAG(IS_MAC)
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&FileSelectHelper::ProcessSelectedFilesMac, this, files));
#else
  ConvertToFileChooserFileInfoList(files);
#endif  // BUILDFLAG(IS_MAC)
}

void FileSelectHelper::MultiFilesSelected(
    const std::vector<base::FilePath>& files,
    void* params) {
  std::vector<ui::SelectedFileInfo> selected_files =
      ui::FilePathListToSelectedFileInfoList(files);

  MultiFilesSelectedWithExtraInfo(selected_files, params);
}

void FileSelectHelper::MultiFilesSelectedWithExtraInfo(
    const std::vector<ui::SelectedFileInfo>& files,
    void* params) {
#if BUILDFLAG(IS_MAC)
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&FileSelectHelper::ProcessSelectedFilesMac, this, files));
#else
  ConvertToFileChooserFileInfoList(files);
#endif  // BUILDFLAG(IS_MAC)
}

void FileSelectHelper::FileSelectionCanceled(void* params) {
  RunFileChooserEnd();
}

void FileSelectHelper::StartNewEnumeration(const base::FilePath& path) {
  base_dir_ = path;
  auto entry = std::make_unique<ActiveDirectoryEnumeration>(path);
  entry->lister_ = base::WrapUnique(new net::DirectoryLister(
      path, net::DirectoryLister::NO_SORT_RECURSIVE, this));
  entry->lister_->Start();
  directory_enumeration_ = std::move(entry);
}

void FileSelectHelper::OnListFile(
    const net::DirectoryLister::DirectoryListerData& data) {
  // Directory upload only cares about files.
  if (data.info.IsDirectory())
    return;

  directory_enumeration_->results_.push_back(data.path);
}

void FileSelectHelper::LaunchConfirmationDialog(
    const base::FilePath& path,
    std::vector<ui::SelectedFileInfo> selected_files) {
  ConvertToFileChooserFileInfoList(std::move(selected_files));
}

void FileSelectHelper::OnListDone(int error) {
  if (!web_contents_) {
    // Web contents was destroyed under us (probably by closing the tab). We
    // must notify |listener_| and release our reference to
    // ourself. RunFileChooserEnd() performs this.
    RunFileChooserEnd();
    return;
  }

  // This entry needs to be cleaned up when this function is done.
  std::unique_ptr<ActiveDirectoryEnumeration> entry =
      std::move(directory_enumeration_);
  if (error) {
    FileSelectionCanceled(NULL);
    return;
  }

  std::vector<ui::SelectedFileInfo> selected_files =
      ui::FilePathListToSelectedFileInfoList(entry->results_);

  if (dialog_type_ == ui::SelectFileDialog::SELECT_UPLOAD_FOLDER) {
    LaunchConfirmationDialog(entry->path_, std::move(selected_files));
  } else {
    std::vector<FileChooserFileInfoPtr> chooser_files;
    for (const auto& file_path : entry->results_) {
      chooser_files.push_back(FileChooserFileInfo::NewNativeFile(
          blink::mojom::NativeFileInfo::New(file_path, std::u16string())));
    }

    listener_->FileSelected(std::move(chooser_files), base_dir_,
                            FileChooserParams::Mode::kUploadFolder);
    listener_.reset();
    EnumerateDirectoryEnd();
  }
}

void FileSelectHelper::ConvertToFileChooserFileInfoList(
    const std::vector<ui::SelectedFileInfo>& files) {
  if (AbortIfWebContentsDestroyed())
    return;

  std::vector<FileChooserFileInfoPtr> chooser_files;
  for (const auto& file : files) {
    chooser_files.push_back(
        FileChooserFileInfo::NewNativeFile(blink::mojom::NativeFileInfo::New(
            file.local_path,
            base::FilePath(file.display_name).AsUTF16Unsafe())));
  }

  PerformContentAnalysisIfNeeded(std::move(chooser_files));
}

void FileSelectHelper::PerformContentAnalysisIfNeeded(
    std::vector<FileChooserFileInfoPtr> list) {
  if (AbortIfWebContentsDestroyed())
    return;

  NotifyListenerAndEnd(std::move(list));
}

void FileSelectHelper::NotifyListenerAndEnd(
    std::vector<blink::mojom::FileChooserFileInfoPtr> list) {
  listener_->FileSelected(std::move(list), base_dir_, dialog_mode_);
  listener_.reset();

  // No members should be accessed from here on.
  RunFileChooserEnd();
}

void FileSelectHelper::DeleteTemporaryFiles() {
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&DeleteFiles, std::move(temporary_files_)));
}

void FileSelectHelper::CleanUp() {
  if (!temporary_files_.empty()) {
    DeleteTemporaryFiles();

    // Now that the temporary files have been scheduled for deletion, there
    // is no longer any reason to keep this instance around.
    Release();
  }
}

bool FileSelectHelper::AbortIfWebContentsDestroyed() {
  if (render_frame_host_ == nullptr || web_contents_ == nullptr) {
    RunFileChooserEnd();
    return true;
  }

  return false;
}

void FileSelectHelper::SetFileSelectListenerForTesting(
    scoped_refptr<content::FileSelectListener> listener) {
  DCHECK(listener);
  DCHECK(!listener_);
  listener_ = std::move(listener);
}

std::unique_ptr<ui::SelectFileDialog::FileTypeInfo>
FileSelectHelper::GetFileTypesFromAcceptType(
    const std::vector<std::u16string>& accept_types) {
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> base_file_type(
      new ui::SelectFileDialog::FileTypeInfo());
  if (accept_types.empty())
    return base_file_type;

  // Create FileTypeInfo and pre-allocate for the first extension list.
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> file_type(
      new ui::SelectFileDialog::FileTypeInfo(*base_file_type));
  file_type->include_all_files = true;
  file_type->extensions.resize(1);
  std::vector<base::FilePath::StringType>* extensions =
      &file_type->extensions.back();

  // Find the corresponding extensions.
  int valid_type_count = 0;
  int description_id = 0;
  for (const auto& accept_type : accept_types) {
    size_t old_extension_size = extensions->size();
    if (accept_type[0] == '.') {
      // If the type starts with a period it is assumed to be a file extension
      // so we just have to add it to the list.
      base::FilePath::StringType ext =
          base::FilePath::FromUTF16Unsafe(accept_type).value();
      extensions->push_back(ext.substr(1));
    } else {
      if (!base::IsStringASCII(accept_type))
        continue;
      std::string ascii_type = base::UTF16ToASCII(accept_type);
      if (ascii_type == "image/*")
        description_id = IDS_IMAGE_FILES;
      else if (ascii_type == "audio/*")
        description_id = IDS_AUDIO_FILES;
      else if (ascii_type == "video/*")
        description_id = IDS_VIDEO_FILES;

      net::GetExtensionsForMimeType(ascii_type, extensions);
    }

    if (extensions->size() > old_extension_size)
      valid_type_count++;
  }

  // If no valid extension is added, bail out.
  if (valid_type_count == 0)
    return base_file_type;

  // Use a generic description "Custom Files" if either of the following is
  // true:
  // 1) There're multiple types specified, like "audio/*,video/*"
  // 2) There're multiple extensions for a MIME type without parameter, like
  //    "ehtml,shtml,htm,html" for "text/html". On Windows, the select file
  //    dialog uses the first extension in the list to form the description,
  //    like "EHTML Files". This is not what we want.
  if (valid_type_count > 1 ||
      (valid_type_count == 1 && description_id == 0 && extensions->size() > 1))
    description_id = IDS_CUSTOM_FILES;

  if (description_id) {
    file_type->extension_description_overrides.push_back(
        l10n_util::GetStringUTF16(description_id));
  }

  return file_type;
}

// static
void FileSelectHelper::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const FileChooserParams& params) {
  // FileSelectHelper will keep itself alive until it sends the result
  // message.
  scoped_refptr<FileSelectHelper> file_select_helper(new FileSelectHelper());
  file_select_helper->RunFileChooser(render_frame_host, std::move(listener),
                                     params.Clone());
}

// static
void FileSelectHelper::EnumerateDirectory(
    content::WebContents* tab,
    scoped_refptr<content::FileSelectListener> listener,
    const base::FilePath& path) {
  // FileSelectHelper will keep itself alive until it sends the result
  // message.
  scoped_refptr<FileSelectHelper> file_select_helper(new FileSelectHelper());
  file_select_helper->EnumerateDirectoryImpl(tab, std::move(listener), path);
}

void FileSelectHelper::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    FileChooserParamsPtr params) {
  DCHECK(!render_frame_host_);
  DCHECK(!web_contents_);
  DCHECK(listener);
  DCHECK(!listener_);
  DCHECK(params->default_file_name.empty() ||
         params->mode == FileChooserParams::Mode::kSave)
      << "The default_file_name parameter should only be specified for Save "
         "file choosers";
  DCHECK(params->default_file_name == params->default_file_name.BaseName())
      << "The default_file_name parameter should not contain path separators";

  render_frame_host_ = render_frame_host;
  web_contents_ = WebContents::FromRenderFrameHost(render_frame_host);
  listener_ = std::move(listener);
  observation_.Reset();
  content::WebContentsObserver::Observe(web_contents_);
  observation_.Observe(render_frame_host_->GetRenderViewHost()->GetWidget());

  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&FileSelectHelper::GetFileTypesInThreadPool, this,
                     std::move(params)));

  // Because this class returns notifications to the RenderViewHost, it is
  // difficult for callers to know how long to keep a reference to this
  // instance. We AddRef() here to keep the instance alive after we return
  // to the caller, until the last callback is received from the file dialog.
  // At that point, we must call RunFileChooserEnd().
  AddRef();
}

void FileSelectHelper::GetFileTypesInThreadPool(FileChooserParamsPtr params) {
  select_file_types_ = GetFileTypesFromAcceptType(params->accept_types);
  select_file_types_->allowed_paths =
      params->need_local_path ? ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH
                              : ui::SelectFileDialog::FileTypeInfo::ANY_PATH;

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&FileSelectHelper::GetSanitizedFilenameOnUIThread, this,
                     std::move(params)));
}

void FileSelectHelper::GetSanitizedFilenameOnUIThread(
    FileChooserParamsPtr params) {
  if (AbortIfWebContentsDestroyed())
    return;

  auto* browser_context = static_cast<electron::ElectronBrowserContext*>(
      render_frame_host_->GetProcess()->GetBrowserContext());
  base::FilePath default_file_path =
      browser_context->prefs()
          ->GetFilePath(prefs::kSelectFileLastDirectory)
          .Append(params->default_file_name);

  RunFileChooserOnUIThread(default_file_path, std::move(params));
}

void FileSelectHelper::RunFileChooserOnUIThread(
    const base::FilePath& default_file_path,
    FileChooserParamsPtr params) {
  DCHECK(params);

  select_file_dialog_ = ui::SelectFileDialog::Create(this, nullptr);
  if (!select_file_dialog_.get())
    return;

  dialog_mode_ = params->mode;
  switch (params->mode) {
    case FileChooserParams::Mode::kOpen:
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_FILE;
      break;
    case FileChooserParams::Mode::kOpenMultiple:
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE;
      break;
    case FileChooserParams::Mode::kUploadFolder:
      dialog_type_ = ui::SelectFileDialog::SELECT_UPLOAD_FOLDER;
      break;
    case FileChooserParams::Mode::kSave:
      dialog_type_ = ui::SelectFileDialog::SELECT_SAVEAS_FILE;
      break;
    default:
      // Prevent warning.
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_FILE;
      NOTREACHED();
  }

  auto* web_contents = electron::api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host_));
  if (!web_contents || !web_contents->owner_window())
    return;

  // Never consider the current scope as hung. The hang watching deadline (if
  // any) is not valid since the user can take unbounded time to choose the
  // file.
  base::HangWatcher::InvalidateActiveExpectations();

  select_file_dialog_->SelectFile(
      dialog_type_, params->title, default_file_path, select_file_types_.get(),
      select_file_types_.get() && !select_file_types_->extensions.empty()
          ? 1
          : 0,  // 1-based index of default extension to show.
      base::FilePath::StringType(),
      web_contents->owner_window()->GetNativeWindow(), NULL);

  select_file_types_.reset();
}

// This method is called when we receive the last callback from the file chooser
// dialog or if the renderer was destroyed. Perform any cleanup and release the
// reference we added in RunFileChooser().
void FileSelectHelper::RunFileChooserEnd() {
  // If there are temporary files, then this instance needs to stick around
  // until web_contents_ is destroyed, so that this instance can delete the
  // temporary files.
  if (!temporary_files_.empty())
    return;

  if (listener_)
    listener_->FileSelectionCanceled();
  render_frame_host_ = nullptr;
  web_contents_ = nullptr;
  Release();
}

void FileSelectHelper::EnumerateDirectoryImpl(
    content::WebContents* tab,
    scoped_refptr<content::FileSelectListener> listener,
    const base::FilePath& path) {
  DCHECK(listener);
  DCHECK(!listener_);
  dialog_type_ = ui::SelectFileDialog::SELECT_NONE;
  web_contents_ = tab;
  listener_ = std::move(listener);
  // Because this class returns notifications to the RenderViewHost, it is
  // difficult for callers to know how long to keep a reference to this
  // instance. We AddRef() here to keep the instance alive after we return
  // to the caller, until the last callback is received from the enumeration
  // code. At that point, we must call EnumerateDirectoryEnd().
  AddRef();
  StartNewEnumeration(path);
}

// This method is called when we receive the last callback from the enumeration
// code. Perform any cleanup and release the reference we added in
// EnumerateDirectoryImpl().
void FileSelectHelper::EnumerateDirectoryEnd() {
  Release();
}

void FileSelectHelper::RenderWidgetHostDestroyed(
    content::RenderWidgetHost* widget_host) {
  render_frame_host_ = nullptr;
  DCHECK(observation_.IsObservingSource(widget_host));
  observation_.Reset();
}

void FileSelectHelper::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  if (!render_frame_host_)
    return;
  // The |old_host| and its children are now pending deletion. Do not give them
  // file access past this point.
  for (content::RenderFrameHost* host = render_frame_host_; host;
       host = host->GetParentOrOuterDocument()) {
    if (host == old_host) {
      render_frame_host_ = nullptr;
      return;
    }
  }
}

void FileSelectHelper::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == render_frame_host_)
    render_frame_host_ = nullptr;
}

void FileSelectHelper::WebContentsDestroyed() {
  render_frame_host_ = nullptr;
  web_contents_ = nullptr;
  CleanUp();
}

// static
bool FileSelectHelper::IsAcceptTypeValid(const std::string& accept_type) {
  // TODO(raymes): This only does some basic checks, extend to test more cases.
  // A 1 character accept type will always be invalid (either a "." in the case
  // of an extension or a "/" in the case of a MIME type).
  std::string unused;
  if (accept_type.length() <= 1 ||
      base::ToLowerASCII(accept_type) != accept_type ||
      base::TrimWhitespaceASCII(accept_type, base::TRIM_ALL, &unused) !=
          base::TRIM_NONE) {
    return false;
  }
  return true;
}

// static
base::FilePath FileSelectHelper::GetSanitizedFileName(
    const base::FilePath& suggested_filename) {
  if (suggested_filename.empty())
    return base::FilePath();
  return net::GenerateFileName(
      GURL(), std::string(), std::string(), suggested_filename.AsUTF8Unsafe(),
      std::string(), l10n_util::GetStringUTF8(IDS_DEFAULT_DOWNLOAD_FILENAME));
}
