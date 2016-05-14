// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_native_image.h"

#include <string>
#include <vector>

#include "atom/common/node_includes.h"
#include "base/process/process_metrics.h"
#include "native_mate/dictionary.h"

namespace {

v8::Local<v8::Value> GetProcessMemoryInfo(v8::Isolate* isolate) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  std::unique_ptr<base::ProcessMetrics> metrics(
    base::ProcessMetrics::CreateCurrentProcessMetrics());

  dict.Set("workingSetSize",
    static_cast<double>(metrics->GetWorkingSetSize() >> 10));
  dict.Set("peakWorkingSetSize",
    static_cast<double>(metrics->GetPeakWorkingSetSize() >> 10));

  size_t private_bytes, shared_bytes;
  if (metrics->GetMemoryBytes(&private_bytes, &shared_bytes)) {
    dict.Set("privateBytes", static_cast<double>(private_bytes >> 10));
    dict.Set("sharedBytes", static_cast<double>(shared_bytes >> 10));
  }

  return dict.GetHandle();
}

v8::Local<v8::Value> GetSystemMemoryInfo(
    v8::Isolate* isolate,
    mate::Arguments* args) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  base::SystemMemoryInfoKB memInfo;

  if (!base::GetSystemMemoryInfo(&memInfo)) {
    args->ThrowError("Unable to retrieve system memory information");
    return v8::Undefined(isolate);
  }

  dict.Set("total", memInfo.total);
  dict.Set("free", memInfo.free);

  // NB: These return bogus values on OS X
#if !defined(OS_MACOSX)
  dict.Set("swapTotal", memInfo.swap_total);
  dict.Set("swapFree", memInfo.swap_free);
#endif

  return dict.GetHandle();
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getProcessMemoryInfo", &GetProcessMemoryInfo);
  dict.SetMethod("getSystemMemoryInfo", &GetSystemMemoryInfo);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_process_stats, Initialize)
