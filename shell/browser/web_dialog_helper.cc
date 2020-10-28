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
#include "net/base/mime_util.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/file_select_helper.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/file_dialog.h"

using blink::mojom::FileChooserFileInfo;
using blink::mojom::FileChooserFileInfoPtr;
using blink::mojom::FileChooserParams;
using blink::mojom::NativeFileInfo;

namespace {

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
    const FileChooserParams& params) {
  file_dialog::DialogSettings settings;
  settings.force_detached = offscreen_;
  settings.filters = GetFileTypesFromAcceptType(params.accept_types);
  settings.parent_window = window_;
  settings.title = base::UTF16ToUTF8(params.title);

  auto* fsc =
      new FileSelectHelper(render_frame_host, std::move(listener), params.mode);

  if (params.mode == FileChooserParams::Mode::kSave) {
    settings.default_path = params.default_file_name;
    fsc->ShowSaveDialog(settings);
  } else {
    int flags = file_dialog::OPEN_DIALOG_CREATE_DIRECTORY;
    switch (params.mode) {
      case FileChooserParams::Mode::kOpenMultiple:
        flags |= file_dialog::OPEN_DIALOG_MULTI_SELECTIONS;
        FALLTHROUGH;
      case FileChooserParams::Mode::kOpen:
        flags |= file_dialog::OPEN_DIALOG_OPEN_FILE;
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
    fsc->ShowOpenDialog(settings);
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
        NativeFileInfo::New(path, base::string16())));
  }

  listener->FileSelected(std::move(file_info), dir,
                         FileChooserParams::Mode::kUploadFolder);
}

}  // namespace electron
