// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_FILEICON_FETCHER_H_
#define ATOM_COMMON_API_FILEICON_FETCHER_H_

#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/icon_manager.h"
#include "content/public/browser/url_data_source.h"

namespace gfx {
class Image;
}

class FileIconFetcher {
  using IconFetchedCallback = base::Callback<void(gfx::Image&)>;
  public:
  static void FetchFileIcon(const base::FilePath& path,
                            float scale_factor,
                            IconLoader::IconSize icon_size,
                            const IconFetchedCallback& callback);

  private:
  struct IconRequestDetails {
    IconRequestDetails();
    IconRequestDetails(const IconRequestDetails& other);
    ~IconRequestDetails();

    // The callback to run with the response.
    IconFetchedCallback callback;

    // The requested scale factor to respond with.
    float scale_factor;
  };
  // Called when favicon data is available from the history backend.
  static void OnFileIconDataAvailable(const IconRequestDetails& details,
                                      gfx::Image* icon);

  // Tracks tasks requesting file icons.
  static base::CancelableTaskTracker cancelable_task_tracker_;
};

#endif  // ATOM_COMMON_API_FILEICON_FETCHER_H_
