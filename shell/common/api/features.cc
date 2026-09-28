// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "printing/buildflags/buildflags.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace {

bool IsBuiltinSpellCheckerEnabled() {
  return BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER);
}

bool IsPDFViewerEnabled() {
  return BUILDFLAG(ENABLE_PDF_VIEWER);
}

bool IsFakeLocationProviderEnabled() {
  return BUILDFLAG(OVERRIDE_LOCATION_PROVIDER);
}

bool IsPrintingEnabled() {
  return BUILDFLAG(ENABLE_PRINTING);
}

bool IsPromptAPIEnabled() {
  return BUILDFLAG(ENABLE_PROMPT_API);
}

bool IsRunAsNodeEnabled() {
  return electron::fuses::IsRunAsNodeEnabled();
}

bool IsComponentBuild() {
#if defined(COMPONENT_BUILD)
  return true;
#else
  return false;
#endif
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod<&IsBuiltinSpellCheckerEnabled>("isBuiltinSpellCheckerEnabled");
  dict.SetMethod<&IsPDFViewerEnabled>("isPDFViewerEnabled");
  dict.SetMethod<&IsFakeLocationProviderEnabled>(
      "isFakeLocationProviderEnabled");
  dict.SetMethod<&IsPrintingEnabled>("isPrintingEnabled");
  dict.SetMethod<&IsPromptAPIEnabled>("isPromptAPIEnabled");
  dict.SetMethod<&IsComponentBuild>("isComponentBuild");
  dict.SetMethod<&IsRunAsNodeEnabled>("isRunAsNodeEnabled");
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_features, Initialize)
