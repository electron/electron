// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_UTIL_GTK_H_
#define ATOM_BROWSER_UI_UTIL_GTK_H_

namespace util_gtk {

/* These are `const char*` rather than the project-preferred `const char[]`
   because they must fit the type of an external dependency */
extern const char* const kCancelLabel;
extern const char* const kNoLabel;
extern const char* const kOkLabel;
extern const char* const kOpenLabel;
extern const char* const kSaveLabel;
extern const char* const kYesLabel;

}  // namespace util_gtk

#endif  // ATOM_BROWSER_UI_UTIL_GTK_H_
