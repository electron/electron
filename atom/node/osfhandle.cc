// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "osfhandle.h"

#include <io.h>

#include "v8-profiler.h"
#include "v8-inspector.h"

namespace node {

int open_osfhandle(intptr_t osfhandle, int flags) {
  return _open_osfhandle(osfhandle, flags);
}

int close(int fd) {
  return _close(fd);
}

void ReferenceSymbols() {
  // Following symbols are used by electron.exe but got stripped by compiler,
  // for some reason, adding them to ForceSymbolReferences does not work,
  // probably because of VC++ bugs.
  v8::TracingCpuProfiler::Create(nullptr);
  reinterpret_cast<v8_inspector::V8InspectorClient*>(nullptr)->unmuteMetrics(0);
}

}  // namespace node
