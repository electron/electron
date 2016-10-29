// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/fileicon_source.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted_memory.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "net/base/escape.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

void FileIconFetcher::FetchFileIcon(const base::FilePath& path,
                                    float scale_factor,
                                    IconLoader::IconSize icon_size,
                                    const IconFetchedCallback& callback) {
  IconManager* im = g_browser_process->icon_manager();
  gfx::Image* icon = im->LookupIconFromFilepath(path, icon_size);

  if (icon) {
    scoped_refptr<base::RefCountedBytes> icon_data(new base::RefCountedBytes);
    gfx::PNGCodec::EncodeBGRASkBitmap(
        icon->ToImageSkia()->GetRepresentation(scale_factor).sk_bitmap(),
        false,
        &icon_data->data());

    callback.Run(icon_data.get());
  } else {
    // Attach the ChromeURLDataManager request ID to the history request.
    IconRequestDetails details;
    details.callback = callback;
    details.scale_factor = scale_factor;

    // Icon was not in cache, go fetch it slowly.
    im->LoadIcon(path,
                 icon_size,
                 base::Bind(&FileIconFetcher::OnFileIconDataAvailable, details),
                 &cancelable_task_tracker_);
  }
}

void FileIconFetcher::OnFileIconDataAvailable(const IconRequestDetails& details,
                                              gfx::Image* icon) {
  if (icon) {
    scoped_refptr<base::RefCountedBytes> icon_data(new base::RefCountedBytes);
    gfx::PNGCodec::EncodeBGRASkBitmap(
        icon->ToImageSkia()->GetRepresentation(
            details.scale_factor).sk_bitmap(),
        false,
        &icon_data->data());

    details.callback.Run(icon_data.get());
  } else {
    details.callback.Run(NULL);
  }
}


FileIconFetcher::IconRequestDetails::IconRequestDetails() : scale_factor(1.0f) {
}

FileIconFetcher::IconRequestDetails::IconRequestDetails(
    const IconRequestDetails& other) = default;

FileIconFetcher::IconRequestDetails::~IconRequestDetails() {
}
