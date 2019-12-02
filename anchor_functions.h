// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_LIBRARY_LOADER_ANCHOR_FUNCTIONS_H_
#define BASE_ANDROID_LIBRARY_LOADER_ANCHOR_FUNCTIONS_H_

#include <stddef.h>
#include <cstdint>

// Start and end of .text, respectively.
extern const size_t kStartOfText;
extern const size_t kEndOfText;
// Start and end of the ordered part of .text, respectively.
extern const size_t kStartOfOrderedText;
extern const size_t kEndOfOrderedText;

// Returns true if anchors are sane.
bool AreAnchorsSane();

// Returns true if the ordering looks sane.
bool IsOrderingSane();

#endif  // BASE_ANDROID_LIBRARY_LOADER_ANCHOR_FUNCTIONS_H_
