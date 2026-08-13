// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_V8_OOM_DIAGNOSTICS_H_
#define ELECTRON_SHELL_COMMON_V8_OOM_DIAGNOSTICS_H_

#include <string_view>

namespace v8 {
class Isolate;
struct OOMDetails;
}  // namespace v8

// Owner of every electron.v8-oom.* crash key. Storage is fixed and statically
// registered, so nothing here allocates and the keys are safe to write from
// V8's OOM and near-heap-limit callbacks on any thread. The first isolate to
// report an OOM owns the keys; other isolates' calls write nothing, so
// concurrent OOMs never interleave into mixed data.
namespace electron::v8_oom {

// Records |isolate|'s heap statistics. Returns false, writing nothing, when
// another isolate already owns the keys.
bool RecordHeapDiagnostics(v8::Isolate* isolate);

// Records the OOM error site reported to V8's OOM error callback.
void RecordErrorDetails(v8::Isolate* isolate,
                        const char* location,
                        const v8::OOMDetails& details);

// Stores the captured JavaScript stack for the owning isolate.
void RecordJsStack(v8::Isolate* isolate, std::string_view stack);

}  // namespace electron::v8_oom

#endif  // ELECTRON_SHELL_COMMON_V8_OOM_DIAGNOSTICS_H_
