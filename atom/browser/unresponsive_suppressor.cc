// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/unresponsive_suppressor.h"

namespace atom {

namespace {

int g_suppress_level = 0;

}  // namespace

bool IsUnresponsiveEventSuppressed() {
  return g_suppress_level > 0;
}

UnresponsiveSuppressor::UnresponsiveSuppressor() {
  g_suppress_level++;
}

UnresponsiveSuppressor::~UnresponsiveSuppressor() {
  g_suppress_level--;
}

}  // namespace atom
