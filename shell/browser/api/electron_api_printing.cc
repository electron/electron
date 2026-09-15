// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "gin/converter.h"
#include "printing/buildflags/buildflags.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "base/functional/bind.h"
#include "printing/backend/print_backend.h"
#include "shell/browser/printing/printing_utils.h"
#include "shell/common/gin_helper/promise.h"
#endif

namespace gin {

#if BUILDFLAG(ENABLE_PRINTING)
template <>
struct Converter<printing::PrinterBasicInfo> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const printing::PrinterBasicInfo& val) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("name", val.printer_name);
    dict.Set("displayName", val.display_name);
    dict.Set("description", val.printer_description);
    dict.Set("options", val.options);
    return dict.GetHandle();
  }
};
#endif

}  // namespace gin

namespace electron::api {

#if BUILDFLAG(ENABLE_PRINTING)
v8::Local<v8::Promise> GetPrinterListAsync(v8::Isolate* isolate) {
  gin_helper::Promise<printing::PrinterList> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  GetPrinterList(base::BindOnce(
      &gin_helper::Promise<printing::PrinterList>::ResolvePromise,
      std::move(promise)));
  return handle;
}

#endif

}  // namespace electron::api

namespace {

#if BUILDFLAG(ENABLE_PRINTING)
using electron::api::GetPrinterListAsync;
#endif

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
#if BUILDFLAG(ENABLE_PRINTING)
  dict.SetMethod("getPrinterListAsync",
                 base::BindRepeating(&GetPrinterListAsync));
#endif
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_printing, Initialize)
