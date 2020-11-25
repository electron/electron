// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_clipboard.h"

#include <shellapi.h>
#include <shlobj.h>

#include "base/win/scoped_hglobal.h"

namespace electron {

namespace api {

std::vector<base::FilePath> Clipboard::ReadFilePaths() {
  std::vector<base::FilePath> result;
  ::OpenClipboard(NULL);
  base::win::ScopedHGlobal<HDROP> hdrop(::GetClipboardData(CF_HDROP));
  if (hdrop.get()) {
    const int kMaxFilenameLen = 4096;
    const unsigned num_files = ::DragQueryFileW(hdrop.get(), 0xffffffff, 0, 0);
    result.reserve(num_files);
    for (unsigned int i = 0; i < num_files; ++i) {
      wchar_t filename[kMaxFilenameLen];
      if (!::DragQueryFileW(hdrop.get(), i, filename, kMaxFilenameLen))
        continue;
      result.push_back(base::FilePath(filename));
    }
  }
  ::CloseClipboard();
  return result;
}

void Clipboard::WriteFilePaths(const std::vector<base::FilePath>& paths) {
  // CF_HDROP clipboard format consists of DROPFILES structure, a series of file
  // names including the terminating null character and the additional null
  // character at the tail to terminate the array.
  // For example,
  //| DROPFILES | FILENAME 1 | NULL | ... | FILENAME n | NULL | NULL |
  // For more details, please refer to
  // https://docs.microsoft.com/ko-kr/windows/desktop/shell/clipboard#cf_hdrop
  size_t total_bytes = sizeof(DROPFILES);
  for (const auto& filename : paths) {
    // Allocate memory of the filename's length including the null
    // character.
    total_bytes += (filename.value().length() + 1) * sizeof(wchar_t);
  }
  // |data| needs to be terminated by an additional null character.
  total_bytes += sizeof(wchar_t);

  // GHND combines GMEM_MOVEABLE and GMEM_ZEROINIT, and GMEM_ZEROINIT
  // initializes memory contents to zero.
  HANDLE hdata = ::GlobalAlloc(GHND, total_bytes);
  if (!hdata)
    return;

  {
    base::win::ScopedHGlobal<DROPFILES*> locked_mem(hdata);
    DROPFILES* drop_files = locked_mem.get();
    drop_files->pFiles = sizeof(DROPFILES);
    drop_files->fWide = TRUE;

    wchar_t* data = reinterpret_cast<wchar_t*>(
        reinterpret_cast<BYTE*>(drop_files) + sizeof(DROPFILES));

    size_t next_filename_offset = 0;
    for (const auto& filename : paths) {
      wcscpy(data + next_filename_offset, filename.value().c_str());
      // Skip the terminating null character of the filename.
      next_filename_offset += filename.value().length() + 1;
    }
  }

  ::OpenClipboard(NULL);
  if (!::SetClipboardData(CF_HDROP, hdata))
    ::GlobalFree(hdata);  // only free data when failed to set clipboard
  ::CloseClipboard();
}

}  // namespace api

}  // namespace electron
