// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef SHELL_BROWSER_WIN_SCOPED_HSTRING_H_
#define SHELL_BROWSER_WIN_SCOPED_HSTRING_H_

#include <hstring.h>
#include <windows.h>

#include <string>

#include "base/macros.h"

namespace electron {

class ScopedHString {
 public:
  // Copy from |source|.
  explicit ScopedHString(const wchar_t* source);
  explicit ScopedHString(const std::wstring& source);
  // Create empty string.
  ScopedHString();
  ~ScopedHString();

  // Sets to |source|.
  void Reset();
  void Reset(const wchar_t* source);
  void Reset(const std::wstring& source);

  // Returns string.
  operator HSTRING() const { return str_; }

  // Whether there is a string created.
  bool success() const { return str_; }

 private:
  HSTRING str_;

  DISALLOW_COPY_AND_ASSIGN(ScopedHString);
};

}  // namespace electron

#endif  // SHELL_BROWSER_WIN_SCOPED_HSTRING_H_
