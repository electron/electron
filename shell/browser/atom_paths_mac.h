// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ATOM_PATHS_MAC_H_
#define SHELL_BROWSER_ATOM_PATHS_MAC_H_

namespace base {
class FilePath;
}

namespace electron {

void GetMacAppLogsPath(base::FilePath* path);

}  // namespace electron
