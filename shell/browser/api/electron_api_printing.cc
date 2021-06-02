// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/threading/thread_restrictions.h"
#include "chrome/browser/browser_process.h"
#include "gin/converter.h"
#include "printing/buildflags/buildflags.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "printing/backend/print_backend.h"
#endif

namespace gin {

#if BUILDFLAG(ENABLE_PRINTING)
template <>
struct Converter<printing::PrinterBasicInfo> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const printing::PrinterBasicInfo& val) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.Set("name", val.printer_name);
    dict.Set("displayName", val.display_name);
    dict.Set("description", val.printer_description);
    dict.Set("status", val.printer_status);
    dict.Set("isDefault", val.is_default ? true : false);
    dict.Set("options", val.options);
    return dict.GetHandle();
  }
};
#endif

}  // namespace gin

namespace electron {

namespace api {

#if BUILDFLAG(ENABLE_PRINTING)
printing::PrinterList GetPrinterList() {
  printing::PrinterList printers;
  auto print_backend = printing::PrintBackend::CreateInstance(
      g_browser_process->GetApplicationLocale());
  {
    // TODO(deepak1556): Deprecate this api in favor of an
    // async version and post a non blocing task call.
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    print_backend->EnumeratePrinters(&printers);
  }
  return printers;
}
#endif

}  // namespace api

}  // namespace electron

namespace {

#if BUILDFLAG(ENABLE_PRINTING)
using electron::api::GetPrinterList;
#endif

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
#if BUILDFLAG(ENABLE_PRINTING)
  dict.SetMethod("getPrinterList", base::BindRepeating(&GetPrinterList));
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_printing, Initialize)
