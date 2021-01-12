// Copyright (c) 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <memory>

#include <string>
#include <utility>
#include <vector>

#include "shell/browser/file_select_helper.h"

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"

using blink::mojom::FileChooserFileInfo;
using blink::mojom::FileChooserFileInfoPtr;
using blink::mojom::FileChooserParams;
using blink::mojom::NativeFileInfo;

namespace {
void DeleteFiles(std::vector<base::FilePath> paths) {
  for (auto& file_path : paths)
    base::DeleteFile(file_path);
}
}  // namespace

FileSelectHelper::FileSelectHelper(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    FileChooserParams::Mode mode)
    : render_frame_host_(render_frame_host),
      listener_(std::move(listener)),
      mode_(mode) {
  DCHECK(render_frame_host_);
  DCHECK(listener_);

  web_contents_ = content::WebContents::FromRenderFrameHost(render_frame_host);
  DCHECK(web_contents_);

  content::WebContentsObserver::Observe(web_contents_);
  observer_.Add(render_frame_host_->GetRenderViewHost()->GetWidget());
}

FileSelectHelper::~FileSelectHelper() = default;

void FileSelectHelper::ShowOpenDialog(
    const file_dialog::DialogSettings& settings) {
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);

  auto callback = base::BindOnce(&FileSelectHelper::OnOpenDialogDone,
                                 weak_ptr_factory_.GetWeakPtr());
  ignore_result(promise.Then(std::move(callback)));

  file_dialog::ShowOpenDialog(settings, std::move(promise));
}

void FileSelectHelper::ShowSaveDialog(
    const file_dialog::DialogSettings& settings) {
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);

  auto callback = base::BindOnce(&FileSelectHelper::OnSaveDialogDone,
                                 weak_ptr_factory_.GetWeakPtr());
  ignore_result(promise.Then(std::move(callback)));

  file_dialog::ShowSaveDialog(settings, std::move(promise));
}

// net::DirectoryLister::DirectoryListerDelegate
void FileSelectHelper::OnListFile(
    const net::DirectoryLister::DirectoryListerData& data) {
  if (!render_frame_host_ || !web_contents_) {
    // If the frame or webcontents was destroyed under us. We
    // must notify |listener_| and release our reference to
    // ourself. RunFileChooserEnd() performs this.
    RunFileChooserEnd();
    return;
  }
  // We don't want to return directory paths, only file paths
  if (data.info.IsDirectory())
    return;

  lister_paths_.push_back(data.path);
}

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

  delete this;
}

// net::DirectoryLister::DirectoryListerDelegate
void FileSelectHelper::OnListDone(int error) {
  if (!render_frame_host_ || !web_contents_) {
    // If the frame or webcontents was destroyed under us. We
    // must notify |listener_| and release our reference to
    // ourself. RunFileChooserEnd() performs this.
    RunFileChooserEnd();
    return;
  }

  std::vector<FileChooserFileInfoPtr> file_info;
  for (const auto& path : lister_paths_)
    file_info.push_back(FileChooserFileInfo::NewNativeFile(
        NativeFileInfo::New(path, base::string16())));

  OnFilesSelected(std::move(file_info), lister_base_dir_);
}

void FileSelectHelper::DeleteTemporaryFiles() {
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&DeleteFiles, std::move(temporary_files_)));
}

void FileSelectHelper::EnumerateDirectory() {
  // Ensure that this fn is only called once
  DCHECK(!lister_);
  DCHECK(!lister_base_dir_.empty());
  DCHECK(lister_paths_.empty());

  lister_ = std::make_unique<net::DirectoryLister>(
      lister_base_dir_, net::DirectoryLister::NO_SORT_RECURSIVE, this);
  lister_->Start();
}

void FileSelectHelper::OnOpenDialogDone(gin_helper::Dictionary result) {
  bool canceled = true;
  result.Get("canceled", &canceled);

  if (!render_frame_host_ || canceled) {
    RunFileChooserEnd();
  } else {
    std::vector<base::FilePath> paths;
    if (result.Get("filePaths", &paths)) {
      std::vector<ui::SelectedFileInfo> files =
          ui::FilePathListToSelectedFileInfoList(paths);
      // If we are uploading a folder we need to enumerate its contents
      if (mode_ == FileChooserParams::Mode::kUploadFolder &&
          paths.size() >= 1) {
        lister_base_dir_ = paths[0];
        EnumerateDirectory();
      } else {
#if defined(OS_MAC)
        base::ThreadPool::PostTask(
            FROM_HERE,
            {base::MayBlock(),
             base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
            base::BindOnce(&FileSelectHelper::ProcessSelectedFilesMac,
                           base::Unretained(this), files));
#else
        ConvertToFileChooserFileInfoList(files);
#endif
      }

      if (render_frame_host_ && !paths.empty()) {
        auto* browser_context = static_cast<electron::ElectronBrowserContext*>(
            render_frame_host_->GetProcess()->GetBrowserContext());
        browser_context->prefs()->SetFilePath(prefs::kSelectFileLastDirectory,
                                              paths[0].DirName());
      }
    }
  }
}

void FileSelectHelper::ConvertToFileChooserFileInfoList(
    const std::vector<ui::SelectedFileInfo>& files) {
  std::vector<FileChooserFileInfoPtr> file_info;

  for (const auto& file : files) {
    file_info.push_back(FileChooserFileInfo::NewNativeFile(NativeFileInfo::New(
        file.local_path, base::FilePath(file.display_name).AsUTF16Unsafe())));
  }

  OnFilesSelected(std::move(file_info), lister_base_dir_);
}

void FileSelectHelper::OnSaveDialogDone(gin_helper::Dictionary result) {
  std::vector<FileChooserFileInfoPtr> file_info;
  bool canceled = true;
  result.Get("canceled", &canceled);

  if (!render_frame_host_ || canceled) {
    RunFileChooserEnd();
  } else {
    base::FilePath path;
    if (result.Get("filePath", &path)) {
      file_info.push_back(FileChooserFileInfo::NewNativeFile(
          NativeFileInfo::New(path, path.BaseName().AsUTF16Unsafe())));
    }
    // We should only call this if we have not cancelled the dialog.
    OnFilesSelected(std::move(file_info), base::FilePath());
  }
}

void FileSelectHelper::OnFilesSelected(
    std::vector<FileChooserFileInfoPtr> file_info,
    base::FilePath base_dir) {
  if (listener_) {
    listener_->FileSelected(std::move(file_info), base_dir, mode_);
    listener_.reset();
  }

  render_frame_host_ = nullptr;

  delete this;
}

void FileSelectHelper::RenderWidgetHostDestroyed(
    content::RenderWidgetHost* widget_host) {
  render_frame_host_ = nullptr;
  observer_.Remove(widget_host);
}

// content::WebContentsObserver:
void FileSelectHelper::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  if (!render_frame_host_)
    return;
  // The |old_host| and its children are now pending deletion. Do not give
  // them file access past this point.
  if (render_frame_host_ == old_host ||
      render_frame_host_->IsDescendantOf(old_host)) {
    render_frame_host_ = nullptr;
  }
}

// content::WebContentsObserver:
void FileSelectHelper::RenderFrameDeleted(
    content::RenderFrameHost* deleted_host) {
  if (deleted_host == render_frame_host_)
    render_frame_host_ = nullptr;
}

// content::WebContentsObserver:
void FileSelectHelper::WebContentsDestroyed() {
  render_frame_host_ = nullptr;
  web_contents_ = nullptr;

  DeleteTemporaryFiles();

  if (!lister_)
    delete this;
}
