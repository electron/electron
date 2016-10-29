// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ICON_LOADER_H_
#define CHROME_BROWSER_ICON_LOADER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/image/image.h"

#if defined(OS_WIN)
// On Windows, we group files by their extension, with several exceptions:
// .dll, .exe, .ico. See IconManager.h for explanation.
typedef std::wstring IconGroupID;
#elif defined(OS_POSIX)
// On POSIX, we group files by MIME type.
typedef std::string IconGroupID;
#endif

////////////////////////////////////////////////////////////////////////////////
//
// A facility to read a file containing an icon asynchronously in the IO
// thread. Returns the icon in the form of an ImageSkia.
//
////////////////////////////////////////////////////////////////////////////////
class IconLoader : public base::RefCountedThreadSafe<IconLoader> {
 public:
  enum IconSize {
    SMALL = 0,  // 16x16
    NORMAL,     // 32x32
    LARGE,      // Windows: 32x32, Linux: 48x48, Mac: Unsupported
    ALL,        // All sizes available
  };

  class Delegate {
   public:
    // Invoked when an icon group has been read, but before the icon data
    // is read. If the icon is already cached, this method should call and
    // return the results of OnImageLoaded with the cached image.
    virtual bool OnGroupLoaded(IconLoader* source,
                               const IconGroupID& group) = 0;
    // Invoked when an icon has been read. |source| is the IconLoader. If the
    // icon has been successfully loaded, result is non-null. This method must
    // return true if it is taking ownership of the returned image.
    virtual bool OnImageLoaded(IconLoader* source,
                               gfx::Image* result,
                               const IconGroupID& group) = 0;

   protected:
    virtual ~Delegate() {}
  };

  IconLoader(const base::FilePath& file_path,
             IconSize size,
             Delegate* delegate);

  // Start reading the icon on the file thread.
  void Start();

 private:
  friend class base::RefCountedThreadSafe<IconLoader>;

  virtual ~IconLoader();

  // Get the identifying string for the given file. The implementation
  // is in icon_loader_[platform].cc.
  static IconGroupID ReadGroupIDFromFilepath(const base::FilePath& path);

  // Some icons (exe's on windows) can change as they're loaded.
  static bool IsIconMutableFromFilepath(const base::FilePath& path);

  // The thread ReadIcon() should be called on.
  static content::BrowserThread::ID ReadIconThreadID();

  void ReadGroup();
  void OnReadGroup();
  void ReadIcon();

  void NotifyDelegate();

  // The task runner object of the thread in which we notify the delegate.
  scoped_refptr<base::SingleThreadTaskRunner> target_task_runner_;

  base::FilePath file_path_;

  IconGroupID group_;

  IconSize icon_size_;

  std::unique_ptr<gfx::Image> image_;

  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(IconLoader);
};

#endif  // CHROME_BROWSER_ICON_LOADER_H_
