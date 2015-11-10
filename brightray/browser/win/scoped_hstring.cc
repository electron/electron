// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/win/scoped_hstring.h"

#include <winstring.h>

ScopedHString::ScopedHString(const wchar_t* source)
    : str_(nullptr) {
  Set(source);
}

ScopedHString::ScopedHString(const std::wstring& source)
    : str_(nullptr) {
  WindowsCreateString(source.c_str(), source.length(), &str_);
}

ScopedHString::ScopedHString() : str_(nullptr) {
}

ScopedHString::~ScopedHString() {
  if (str_)
    WindowsDeleteString(str_);
}

void ScopedHString::Set(const wchar_t* source) {
  if (str_) {
    WindowsDeleteString(str_);
    str_ = nullptr;
  }
  WindowsCreateString(source, wcslen(source), &str_);
}
