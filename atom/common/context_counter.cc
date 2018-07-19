// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/context_counter.h"

namespace atom {

namespace {

int g_context_id = 0;

}  // namespace

int GetNextContextId() {
  return ++g_context_id;
}

}  // namespace atom
