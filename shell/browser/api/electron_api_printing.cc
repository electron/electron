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
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "printing/backend/print_backend.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/process_util.h"
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

namespace electron::api {

#if BUILDFLAG(ENABLE_PRINTING)
printing::PrinterList GetPrinterList(v8::Isolate* isolate) {
  EmitWarning(node::Environment::GetCurrent(isolate),
              "Deprecation Warning: getPrinters() is deprecated. "
              "Use the asynchronous and non-blocking version, "
              "getPrintersAsync(), instead.",
              "electron");
  printing::PrinterList printers;
  auto print_backend = printing::PrintBackend::CreateInstance(
      g_browser_process->GetApplicationLocale());
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    printing::mojom::ResultCode code =
        print_backend->EnumeratePrinters(printers);
    if (code != printing::mojom::ResultCode::kSuccess)
      LOG(INFO) << "Failed to enumerate printers";
  }
  return printers;
}

v8::Local<v8::Promise> GetPrinterListAsync(v8::Isolate* isolate) {
  gin_helper::Promise<printing::PrinterList> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_VISIBLE, base::MayBlock()},
      base::BindOnce([]() {
        printing::PrinterList printers;
        auto print_backend = printing::PrintBackend::CreateInstance(
            g_browser_process->GetApplicationLocale());
        printing::mojom::ResultCode code =
            print_backend->EnumeratePrinters(printers);
        if (code != printing::mojom::ResultCode::kSuccess)
          LOG(INFO) << "Failed to enumerate printers";
        return printers;
      }),
      base::BindOnce(
          [](gin_helper::Promise<printing::PrinterList> promise,
             const printing::PrinterList& printers) {
            promise.Resolve(printers);
          },
          std::move(promise)));

  return handle;
}
#endif

}  // namespace electron::api

namespace {

#if BUILDFLAG(ENABLE_PRINTING)
using electron::api::GetPrinterList;
using electron::api::GetPrinterListAsync;
#endif

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
#if BUILDFLAG(ENABLE_PRINTING)
  dict.SetMethod("getPrinterList", base::BindRepeating(&GetPrinterList));
  dict.SetMethod("getPrinterListAsync",
                 base::BindRepeating(&GetPrinterListAsync));
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_printing, Initialize)
