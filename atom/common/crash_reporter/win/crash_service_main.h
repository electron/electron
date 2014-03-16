// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMMON_CRASH_REPORTER_WIN_CRASH_SERVICE_MAIN_H_
#define COMMON_CRASH_REPORTER_WIN_CRASH_SERVICE_MAIN_H_

namespace crash_service {

// Program entry, should be called by main();
int Main(const wchar_t* cmd_line);

}  // namespace crash_service

#endif  // COMMON_CRASH_REPORTER_WIN_CRASH_SERVICE_MAIN_H_
