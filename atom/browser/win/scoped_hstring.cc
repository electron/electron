// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "atom/browser/win/scoped_hstring.h"

#include <winstring.h>

namespace atom {

ScopedHString::ScopedHString(const wchar_t* source) : str_(nullptr) {
  Reset(source);
}

ScopedHString::ScopedHString(const std::wstring& source) : str_(nullptr) {
  Reset(source);
}

ScopedHString::ScopedHString() : str_(nullptr) {}

ScopedHString::~ScopedHString() {
  Reset();
}

void ScopedHString::Reset() {
  if (str_) {
    WindowsDeleteString(str_);
    str_ = nullptr;
  }
}

void ScopedHString::Reset(const wchar_t* source) {
  Reset();
  WindowsCreateString(source, wcslen(source), &str_);
}

void ScopedHString::Reset(const std::wstring& source) {
  Reset();
  WindowsCreateString(source.c_str(), source.length(), &str_);
}

}  // namespace atom
