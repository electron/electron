// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ATOM_COMMON_MAC_MAIN_APPLICATION_BUNDLE_H_
#define ATOM_COMMON_MAC_MAIN_APPLICATION_BUNDLE_H_

@class NSBundle;

namespace base {
class FilePath;
}

namespace atom {

// The "main" application bundle is the outermost bundle for this logical
// application. E.g., if you have MyApp.app and
// MyApp.app/Contents/Frameworks/MyApp Helper.app, the main application bundle
// is MyApp.app, no matter which executable is currently running.
NSBundle* MainApplicationBundle();
base::FilePath MainApplicationBundlePath();

}  // namespace atom

#endif  // ATOM_COMMON_MAC_MAIN_APPLICATION_BUNDLE_H_
