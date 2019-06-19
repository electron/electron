// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_HEAP_SNAPSHOT_H_
#define ATOM_COMMON_HEAP_SNAPSHOT_H_

#include "base/files/file.h"
#include "v8/include/v8.h"

namespace atom {

bool TakeHeapSnapshot(v8::Isolate* isolate, base::File* file);

}  // namespace atom

#endif  // ATOM_COMMON_HEAP_SNAPSHOT_H_
