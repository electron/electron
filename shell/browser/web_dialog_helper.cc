// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/web_dialog_helper.h"

#include <memory>

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "gin/dictionary.h"
#include "net/base/directory_lister.h"
#include "net/base/mime_util.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "ui/shell_dialogs/selected_file_info.h"

using blink::mojom::FileChooserFileInfo;
using blink::mojom::FileChooserFileInfoPtr;
using blink::mojom::FileChooserParams;

namespace {

class FileSelectHelper : public base::RefCounted<FileSelectHelper>,
                         public content::WebContentsObserver,
                         public net::DirectoryLister::DirectoryListerDelegate {
 public:
  REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();

  FileSelectHelper(content::RenderFrameHost* render_frame_host,
                   scoped_refptr<content::FileSelectListener> listener,
                   blink::mojom::FileChooserParams::Mode mode)
      : render_frame_host_(render_frame_host),
        listener_(std::move(listener)),
        mode_(mode) {
    auto* web_contents =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    content::WebContentsObserver::Observe(web_contents);
  }

  void ShowOpenDialog(const file_dialog::DialogSettings& settings) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    gin_helper::Promise<gin_helper::Dictionary> promise(isolate);

    auto callback = base::BindOnce(&FileSelectHelper::OnOpenDialogDone, this);
    ignore_result(promise.Then(std::move(callback)));

    file_dialog::ShowOpenDialog(settings, std::move(promise));
  }

  void ShowSaveDialog(const file_dialog::DialogSettings& settings) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    gin_helper::Promise<gin_helper::Dictionary> promise(isolate);

    auto callback = base::BindOnce(&FileSelectHelper::OnSaveDialogDone, this);
    ignore_result(promise.Then(std::move(callback)));

    file_dialog::ShowSaveDialog(settings, std::move(promise));
  }

 private:
  friend class base::RefCounted<FileSelectHelper>;

  ~FileSelectHelper() override = default;

  // net::DirectoryLister::DirectoryListerDelegate
  void OnListFile(
      const net::DirectoryLister::DirectoryListerData& data) override {
    // We don't want to return directory paths, only file paths
    if (data.info.IsDirectory())
      return;

    lister_paths_.push_back(data.path);
  }

  // net::DirectoryLister::DirectoryListerDelegate
  void OnListDone(int error) override {
    std::vector<FileChooserFileInfoPtr> file_info;
    for (const auto& path : lister_paths_)
      file_info.push_back(FileChooserFileInfo::NewNativeFile(
          blink::mojom::NativeFileInfo::New(path, base::string16())));

    OnFilesSelected(std::move(file_info), lister_base_dir_);
    Release();
  }

  void EnumerateDirectory() {
    // Ensure that this fn is only called once
    DCHECK(!lister_);
    DCHECK(!lister_base_dir_.empty());
    DCHECK(lister_paths_.empty());

    lister_ = std::make_unique<net::DirectoryLister>(
        lister_base_dir_, net::DirectoryLister::NO_SORT_RECURSIVE, this);
    lister_->Start();
    // It is difficult for callers to know how long to keep a reference to
    // this instance.  We AddRef() here to keep the instance alive after we
    // return to the caller.  Once the directory lister is complete we
    // Release() & at that point we run OnFilesSelected() which will
    // deref the last reference held by the listener.
    AddRef();
  }

  void OnOpenDialogDone(gin_helper::Dictionary result) {
    std::vector<FileChooserFileInfoPtr> file_info;
    bool canceled = true;
    result.Get("canceled", &canceled);
    // For certain file chooser modes (kUploadFolder) we need to do some async
    // work before calling back to the listener.  In that particular case the
    // listener is called from the directory enumerator.
    bool ready_to_call_listener = false;

    if (canceled) {
      OnSelectionCancelled();
    } else {
      std::vector<base::FilePath> paths;
      if (result.Get("filePaths", &paths)) {
        // If we are uploading a folder we need to enumerate its contents
        if (mode_ == FileChooserParams::Mode::kUploadFolder &&
            paths.size() >= 1) {
          lister_base_dir_ = paths[0];
          EnumerateDirectory();
        } else {
          for (auto& path : paths) {
            file_info.push_back(FileChooserFileInfo::NewNativeFile(
                blink::mojom::NativeFileInfo::New(
                    path, path.BaseName().AsUTF16Unsafe())));
          }

          ready_to_call_listener = true;
        }

        if (render_frame_host_ && !paths.empty()) {
          auto* browser_context =
              static_cast<electron::ElectronBrowserContext*>(
                  render_frame_host_->GetProcess()->GetBrowserContext());
          browser_context->prefs()->SetFilePath(prefs::kSelectFileLastDirectory,
                                                paths[0].DirName());
        }
      }
      // We should only call this if we have not cancelled the dialog
      if (ready_to_call_listener)
        OnFilesSelected(std::move(file_info), lister_base_dir_);
    }
  }

  void OnSaveDialogDone(gin_helper::Dictionary result) {
    std::vector<FileChooserFileInfoPtr> file_info;
    bool canceled = true;
    result.Get("canceled", &canceled);

    if (canceled) {
      OnSelectionCancelled();
    } else {
      base::FilePath path;
      if (result.Get("filePath", &path)) {
        file_info.push_back(FileChooserFileInfo::NewNativeFile(
            blink::mojom::NativeFileInfo::New(
                path, path.BaseName().AsUTF16Unsafe())));
      }
      // We should only call this if we have not cancelled the dialog
      OnFilesSelected(std::move(file_info), base::FilePath());
    }
  }

  void OnFilesSelected(std::vector<FileChooserFileInfoPtr> file_info,
                       base::FilePath base_dir) {
    if (listener_) {
      listener_->FileSelected(std::move(file_info), base_dir, mode_);
      listener_.reset();
    }
    render_frame_host_ = nullptr;
  }

  void OnSelectionCancelled() {
    if (listener_) {
      listener_->FileSelectionCanceled();
      listener_.reset();
    }
    render_frame_host_ = nullptr;
  }

  // content::WebContentsObserver:
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override {
    if (old_host == render_frame_host_)
      render_frame_host_ = nullptr;
  }

  // content::WebContentsObserver:
  void RenderFrameDeleted(content::RenderFrameHost* deleted_host) override {
    if (deleted_host == render_frame_host_)
      render_frame_host_ = nullptr;
  }

  // content::WebContentsObserver:
  void WebContentsDestroyed() override { render_frame_host_ = nullptr; }

  content::RenderFrameHost* render_frame_host_;
  scoped_refptr<content::FileSelectListener> listener_;
  blink::mojom::FileChooserParams::Mode mode_;

  // DirectoryLister-specific members
  std::unique_ptr<net::DirectoryLister> lister_;
  base::FilePath lister_base_dir_;
  std::vector<base::FilePath> lister_paths_;
};

file_dialog::Filters GetFileTypesFromAcceptType(
    const std::vector<base::string16>& accept_types) {
  file_dialog::Filters filters;
  if (accept_types.empty())
    return filters;

  std::vector<base::FilePath::StringType> extensions;

  int valid_type_count = 0;
  std::string description;

  for (const auto& accept_type : accept_types) {
    std::string ascii_type = base::UTF16ToASCII(accept_type);
    auto old_extension_size = extensions.size();

    if (ascii_type[0] == '.') {
      // If the type starts with a period it is assumed to be a file extension,
      // like `.txt`, // so we just have to add it to the list.
      base::FilePath::StringType extension(ascii_type.begin(),
                                           ascii_type.end());
      // Skip the first character.
      extensions.push_back(extension.substr(1));
    } else {
      if (ascii_type == "image/*")
        description = "Image Files";
      else if (ascii_type == "audio/*")
        description = "Audio Files";
      else if (ascii_type == "video/*")
        description = "Video Files";

      // For MIME Type, `audio/*, video/*, image/*
      net::GetExtensionsForMimeType(ascii_type, &extensions);
    }

    if (extensions.size() > old_extension_size)
      valid_type_count++;
  }

  // If no valid extension is added, return empty filters.
  if (extensions.empty())
    return filters;

  filters.push_back(file_dialog::Filter());

  if (valid_type_count > 1 || (valid_type_count == 1 && description.empty()))
    description = "Custom Files";

  DCHECK(!description.empty());
  filters[0].first = description;

  for (const auto& extension : extensions) {
#if defined(OS_WIN)
    filters[0].second.push_back(base::UTF16ToASCII(extension));
#else
    filters[0].second.push_back(extension);
#endif
  }

  // Allow all files when extension is specified.
  filters.push_back(file_dialog::Filter());
  filters.back().first = "All Files";
  filters.back().second.emplace_back("*");

  return filters;
}

}  // namespace

namespace electron {

WebDialogHelper::WebDialogHelper(NativeWindow* window, bool offscreen)
    : window_(window), offscreen_(offscreen), weak_factory_(this) {}

WebDialogHelper::~WebDialogHelper() = default;

void WebDialogHelper::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params) {
  file_dialog::DialogSettings settings;
  settings.force_detached = offscreen_;
  settings.filters = GetFileTypesFromAcceptType(params.accept_types);
  settings.parent_window = window_;
  settings.title = base::UTF16ToUTF8(params.title);

  auto file_select_helper = base::MakeRefCounted<FileSelectHelper>(
      render_frame_host, std::move(listener), params.mode);
  if (params.mode == FileChooserParams::Mode::kSave) {
    settings.default_path = params.default_file_name;
    file_select_helper->ShowSaveDialog(settings);
  } else {
    int flags = file_dialog::OPEN_DIALOG_CREATE_DIRECTORY;
    switch (params.mode) {
      case FileChooserParams::Mode::kOpenMultiple:
        flags |= file_dialog::OPEN_DIALOG_MULTI_SELECTIONS;
        FALLTHROUGH;
      case FileChooserParams::Mode::kOpen:
        flags |= file_dialog::OPEN_DIALOG_OPEN_FILE;
        flags |= file_dialog::OPEN_DIALOG_TREAT_PACKAGE_APP_AS_DIRECTORY;
        break;
      case FileChooserParams::Mode::kUploadFolder:
        flags |= file_dialog::OPEN_DIALOG_OPEN_DIRECTORY;
        break;
      default:
        NOTREACHED();
    }

    auto* browser_context = static_cast<electron::ElectronBrowserContext*>(
        render_frame_host->GetProcess()->GetBrowserContext());
    settings.default_path = browser_context->prefs()
                                ->GetFilePath(prefs::kSelectFileLastDirectory)
                                .Append(params.default_file_name);
    settings.properties = flags;
    file_select_helper->ShowOpenDialog(settings);
  }
}

void WebDialogHelper::EnumerateDirectory(
    content::WebContents* web_contents,
    scoped_refptr<content::FileSelectListener> listener,
    const base::FilePath& dir) {
  int types = base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES |
              base::FileEnumerator::INCLUDE_DOT_DOT;
  base::FileEnumerator file_enum(dir, false, types);

  base::FilePath path;
  std::vector<FileChooserFileInfoPtr> file_info;
  while (!(path = file_enum.Next()).empty()) {
    file_info.push_back(FileChooserFileInfo::NewNativeFile(
        blink::mojom::NativeFileInfo::New(path, base::string16())));
  }

  listener->FileSelected(std::move(file_info), dir,
                         FileChooserParams::Mode::kUploadFolder);
}

}  // namespace electron
