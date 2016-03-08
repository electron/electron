// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pdf/pdf.h"

#include <stdint.h>

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "base/command_line.h"
#include "base/logging.h"
#include "pdf/out_of_process_instance.h"
#include "ppapi/c/ppp.h"
#include "ppapi/cpp/private/internal_module.h"
#include "ppapi/cpp/private/pdf.h"
#include "v8/include/v8.h"

namespace chrome_pdf {

namespace {

bool g_sdk_initialized_via_pepper = false;

}  // namespace

PDFModule::PDFModule() {
}

PDFModule::~PDFModule() {
  if (g_sdk_initialized_via_pepper) {
    chrome_pdf::ShutdownSDK();
    g_sdk_initialized_via_pepper = false;
  }
}

bool PDFModule::Init() {
  return true;
}

pp::Instance* PDFModule::CreateInstance(PP_Instance instance) {
  if (!g_sdk_initialized_via_pepper) {
    v8::StartupData natives;
    v8::StartupData snapshot;
    pp::PDF::GetV8ExternalSnapshotData(pp::InstanceHandle(instance),
                                       &natives.data, &natives.raw_size,
                                       &snapshot.data, &snapshot.raw_size);
    if (natives.data) {
      v8::V8::SetNativesDataBlob(&natives);
      v8::V8::SetSnapshotDataBlob(&snapshot);
    }
    if (!chrome_pdf::InitializeSDK())
      return NULL;
    g_sdk_initialized_via_pepper = true;
  }

  return new OutOfProcessInstance(instance);
}


// Implementation of Global PPP functions ---------------------------------
int32_t PPP_InitializeModule(PP_Module module_id,
                             PPB_GetInterface get_browser_interface) {
  PDFModule* module = new PDFModule();
  if (!module->InternalInit(module_id, get_browser_interface)) {
    delete module;
    return PP_ERROR_FAILED;
  }

  pp::InternalSetModuleSingleton(module);
  return PP_OK;
}

void PPP_ShutdownModule() {
  delete pp::Module::Get();
  pp::InternalSetModuleSingleton(NULL);
}

const void* PPP_GetInterface(const char* interface_name) {
  if (!pp::Module::Get())
    return NULL;
  return pp::Module::Get()->GetPluginInterface(interface_name);
}

#if defined(OS_WIN)
bool RenderPDFPageToDC(const void* pdf_buffer,
                                 int buffer_size,
                                 int page_number,
                                 HDC dc,
                                 int dpi,
                                 int bounds_origin_x,
                                 int bounds_origin_y,
                                 int bounds_width,
                                 int bounds_height,
                                 bool fit_to_bounds,
                                 bool stretch_to_bounds,
                                 bool keep_aspect_ratio,
                                 bool center_in_bounds,
                                 bool autorotate) {
  if (!g_sdk_initialized_via_pepper) {
    if (!chrome_pdf::InitializeSDK()) {
      return false;
    }
  }
  chrome_pdf::PDFEngineExports* engine_exports =
      chrome_pdf::PDFEngineExports::Get();
  chrome_pdf::PDFEngineExports::RenderingSettings settings(
      dpi, dpi, pp::Rect(bounds_origin_x, bounds_origin_y, bounds_width,
                         bounds_height),
      fit_to_bounds, stretch_to_bounds, keep_aspect_ratio, center_in_bounds,
      autorotate);
  bool ret = engine_exports->RenderPDFPageToDC(pdf_buffer, buffer_size,
                                               page_number, settings, dc);
  if (!g_sdk_initialized_via_pepper) {
    chrome_pdf::ShutdownSDK();
  }
  return ret;
}

#endif  // OS_WIN

bool GetPDFDocInfo(const void* pdf_buffer,
                   int buffer_size, int* page_count,
                   double* max_page_width) {
  if (!g_sdk_initialized_via_pepper) {
    if (!chrome_pdf::InitializeSDK())
      return false;
  }
  chrome_pdf::PDFEngineExports* engine_exports =
      chrome_pdf::PDFEngineExports::Get();
  bool ret = engine_exports->GetPDFDocInfo(
      pdf_buffer, buffer_size, page_count, max_page_width);
  if (!g_sdk_initialized_via_pepper) {
    chrome_pdf::ShutdownSDK();
  }
  return ret;
}

bool GetPDFPageSizeByIndex(const void* pdf_buffer,
                           int pdf_buffer_size, int page_number,
                           double* width, double* height) {
  if (!g_sdk_initialized_via_pepper) {
    if (!chrome_pdf::InitializeSDK())
      return false;
  }
  chrome_pdf::PDFEngineExports* engine_exports =
      chrome_pdf::PDFEngineExports::Get();
  bool ret = engine_exports->GetPDFPageSizeByIndex(
      pdf_buffer, pdf_buffer_size, page_number, width, height);
  if (!g_sdk_initialized_via_pepper)
    chrome_pdf::ShutdownSDK();
  return ret;
}

bool RenderPDFPageToBitmap(const void* pdf_buffer,
                           int pdf_buffer_size,
                           int page_number,
                           void* bitmap_buffer,
                           int bitmap_width,
                           int bitmap_height,
                           int dpi,
                           bool autorotate) {
  if (!g_sdk_initialized_via_pepper) {
    if (!chrome_pdf::InitializeSDK())
      return false;
  }
  chrome_pdf::PDFEngineExports* engine_exports =
      chrome_pdf::PDFEngineExports::Get();
  chrome_pdf::PDFEngineExports::RenderingSettings settings(
      dpi, dpi, pp::Rect(bitmap_width, bitmap_height), true, false, true, true,
      autorotate);
  bool ret = engine_exports->RenderPDFPageToBitmap(
      pdf_buffer, pdf_buffer_size, page_number, settings, bitmap_buffer);
  if (!g_sdk_initialized_via_pepper) {
    chrome_pdf::ShutdownSDK();
  }
  return ret;
}

}  // namespace chrome_pdf
