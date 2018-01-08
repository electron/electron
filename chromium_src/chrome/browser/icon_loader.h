// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ICON_LOADER_H_
#define CHROME_BROWSER_ICON_LOADER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/image/image.h"

////////////////////////////////////////////////////////////////////////////////
//
// A facility to read a file containing an icon asynchronously in the IO
// thread. Returns the icon in the form of an ImageSkia.
//
////////////////////////////////////////////////////////////////////////////////
class IconLoader {
 public:
  // An IconGroup is a class of files that all share the same icon. For all
  // platforms but Windows, and for most files on Windows, it is the file type
  // (e.g. all .mp3 files share an icon, all .html files share an icon). On
  // Windows, for certain file types (.exe, .dll, etc), each file of that type
  // is assumed to have a unique icon. In that case, each of those files is a
  // group to itself.
  using IconGroup = base::FilePath::StringType;

  enum IconSize {
    SMALL = 0,  // 16x16
    NORMAL,     // 32x32
    LARGE,      // Windows: 32x32, Linux: 48x48, Mac: Unsupported
    ALL,        // All sizes available
  };

  // The callback invoked when an icon has been read. The parameters are:
  // - The icon that was loaded, or null if there was a failure to load it.
  // - The determined group from the original requested path.
  using IconLoadedCallback =
      base::Callback<void(std::unique_ptr<gfx::Image>, const IconGroup&)>;

  // Creates an IconLoader, which owns itself. If the IconLoader might outlive
  // the caller, be sure to use a weak pointer in the |callback|.
  static IconLoader* Create(const base::FilePath& file_path,
                            IconSize size,
                            IconLoadedCallback callback);

  // Starts the process of reading the icon. When the reading of the icon is
  // complete, the IconLoadedCallback callback will be fulfilled, and the
  // IconLoader will delete itself.
  void Start();

 private:
  IconLoader(const base::FilePath& file_path,
             IconSize size,
             IconLoadedCallback callback);

  ~IconLoader();

  // Given a file path, get the group for the given file.
  static IconGroup GroupForFilepath(const base::FilePath& file_path);

  // The thread ReadIcon() should be called on.
  static content::BrowserThread::ID ReadIconThreadID();

  void ReadGroup();
  void OnReadGroup();
  void ReadIcon();

  // The task runner object of the thread in which we notify the delegate.
  scoped_refptr<base::SingleThreadTaskRunner> target_task_runner_;

  base::FilePath file_path_;

  IconGroup group_;

  IconSize icon_size_;

  IconLoadedCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(IconLoader);
};

#endif  // CHROME_BROWSER_ICON_LOADER_H_
