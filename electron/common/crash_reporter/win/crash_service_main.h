// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_CRASH_REPORTER_WIN_CRASH_SERVICE_MAIN_H_
#define ATOM_COMMON_CRASH_REPORTER_WIN_CRASH_SERVICE_MAIN_H_

namespace crash_service {

// Program entry, should be called by main();
int Main(const wchar_t* cmd_line);

}  // namespace crash_service

#endif  // ATOM_COMMON_CRASH_REPORTER_WIN_CRASH_SERVICE_MAIN_H_
