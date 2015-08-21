// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_dialog_helper.h"

#include <string>
#include <vector>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/prefs/pref_service.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/file_chooser_file_info.h"
#include "net/base/mime_util.h"
#include "ui/shell_dialogs/selected_file_info.h"

namespace {

file_dialog::Filters GetFileTypesFromAcceptType(
    const std::vector<base::string16>& accept_types) {
  file_dialog::Filters filters;
  if (accept_types.empty())
    return filters;

  std::vector<base::FilePath::StringType> extensions;

  for (const auto& accept_type : accept_types) {
    std::string ascii_type = base::UTF16ToASCII(accept_type);
    if (ascii_type[0] == '.') {
      // If the type starts with a period it is assumed to be a file extension,
      // like `.txt`, // so we just have to add it to the list.
      base::FilePath::StringType extension(
          ascii_type.begin(), ascii_type.end());
      // Skip the first character.
      extensions.push_back(extension.substr(1));
    } else {
      // For MIME Type, `audio/*, vidio/*, image/*
      net::GetExtensionsForMimeType(ascii_type, &extensions);
    }
  }

  // If no valid exntesion is added, return empty filters.
  if (extensions.empty())
    return filters;

  filters.push_back(file_dialog::Filter());
  for (const auto& extension : extensions) {
#if defined(OS_WIN)
    filters[0].second.push_back(base::UTF16ToASCII(extension));
#else
    filters[0].second.push_back(extension);
#endif
  }
  return filters;
}

}  // namespace

namespace atom {

WebDialogHelper::WebDialogHelper(NativeWindow* window)
    : window_(window),
      weak_factory_(this) {
}

WebDialogHelper::~WebDialogHelper() {
}


void WebDialogHelper::RunFileChooser(content::WebContents* web_contents,
                                     const content::FileChooserParams& params) {
  std::vector<content::FileChooserFileInfo> result;
  file_dialog::Filters filters = GetFileTypesFromAcceptType(
      params.accept_types);
  if (params.mode == content::FileChooserParams::Save) {
    base::FilePath path;
    if (file_dialog::ShowSaveDialog(window_,
                                    base::UTF16ToUTF8(params.title),
                                    params.default_file_name,
                                    filters,
                                    &path)) {
      content::FileChooserFileInfo info;
      info.file_path = path;
      info.display_name = path.BaseName().value();
      result.push_back(info);
    }
  } else {
    int flags = file_dialog::FILE_DIALOG_CREATE_DIRECTORY;
    switch (params.mode) {
      case content::FileChooserParams::OpenMultiple:
        flags |= file_dialog::FILE_DIALOG_MULTI_SELECTIONS;
      case content::FileChooserParams::Open:
        flags |= file_dialog::FILE_DIALOG_OPEN_FILE;
        break;
      case content::FileChooserParams::UploadFolder:
        flags |= file_dialog::FILE_DIALOG_OPEN_DIRECTORY;
        break;
      default:
        NOTREACHED();
    }

    std::vector<base::FilePath> paths;
    AtomBrowserContext* browser_context = static_cast<AtomBrowserContext*>(
        window_->web_contents()->GetBrowserContext());
    base::FilePath default_file_path = browser_context->prefs()->GetFilePath(
        prefs::kSelectFileLastDirectory).Append(params.default_file_name);
    if (file_dialog::ShowOpenDialog(window_,
                                    base::UTF16ToUTF8(params.title),
                                    default_file_path,
                                    filters,
                                    flags,
                                    &paths)) {
      for (auto& path : paths) {
        content::FileChooserFileInfo info;
        info.file_path = path;
        info.display_name = path.BaseName().value();
        result.push_back(info);
      }
      if (!paths.empty()) {
        browser_context->prefs()->SetFilePath(prefs::kSelectFileLastDirectory,
                                              paths[0].DirName());
      }
    }
  }

  web_contents->GetRenderViewHost()->FilesSelectedInChooser(
      result, params.mode);
}

void WebDialogHelper::EnumerateDirectory(content::WebContents* web_contents,
                                         int request_id,
                                         const base::FilePath& dir) {
  int types = base::FileEnumerator::FILES |
              base::FileEnumerator::DIRECTORIES |
              base::FileEnumerator::INCLUDE_DOT_DOT;
  base::FileEnumerator file_enum(dir, false, types);

  base::FilePath path;
  std::vector<base::FilePath> paths;
  while (!(path = file_enum.Next()).empty())
    paths.push_back(path);

  web_contents->GetRenderViewHost()->DirectoryEnumerationFinished(
      request_id, paths);
}

}  // namespace atom
