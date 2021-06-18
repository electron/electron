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

bool IsDesktopCapturerEnabled() {
  return BUILDFLAG(ENABLE_DESKTOP_CAPTURER);
}

bool IsOffscreenRenderingEnabled() {
  return BUILDFLAG(ENABLE_OSR);
}

bool IsPDFViewerEnabled() {
  return BUILDFLAG(ENABLE_PDF_VIEWER);
}

bool IsRunAsNodeEnabled() {
  return electron::fuses::IsRunAsNodeEnabled() && BUILDFLAG(ENABLE_RUN_AS_NODE);
}

bool IsFakeLocationProviderEnabled() {
  return BUILDFLAG(OVERRIDE_LOCATION_PROVIDER);
}

bool IsViewApiEnabled() {
  return BUILDFLAG(ENABLE_VIEWS_API);
}

bool IsTtsEnabled() {
  return BUILDFLAG(ENABLE_TTS);
}

bool IsPrintingEnabled() {
  return BUILDFLAG(ENABLE_PRINTING);
}

bool IsExtensionsEnabled() {
  return BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS);
}

bool IsPictureInPictureEnabled() {
  return BUILDFLAG(ENABLE_PICTURE_IN_PICTURE);
}

bool IsWinDarkModeWindowUiEnabled() {
  return BUILDFLAG(ENABLE_WIN_DARK_MODE_WINDOW_UI);
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
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("isBuiltinSpellCheckerEnabled", &IsBuiltinSpellCheckerEnabled);
  dict.SetMethod("isDesktopCapturerEnabled", &IsDesktopCapturerEnabled);
  dict.SetMethod("isOffscreenRenderingEnabled", &IsOffscreenRenderingEnabled);
  dict.SetMethod("isPDFViewerEnabled", &IsPDFViewerEnabled);
  dict.SetMethod("isRunAsNodeEnabled", &IsRunAsNodeEnabled);
  dict.SetMethod("isFakeLocationProviderEnabled",
                 &IsFakeLocationProviderEnabled);
  dict.SetMethod("isViewApiEnabled", &IsViewApiEnabled);
  dict.SetMethod("isTtsEnabled", &IsTtsEnabled);
  dict.SetMethod("isPrintingEnabled", &IsPrintingEnabled);
  dict.SetMethod("isPictureInPictureEnabled", &IsPictureInPictureEnabled);
  dict.SetMethod("isComponentBuild", &IsComponentBuild);
  dict.SetMethod("isExtensionsEnabled", &IsExtensionsEnabled);
  dict.SetMethod("isWinDarkModeWindowUiEnabled", &IsWinDarkModeWindowUiEnabled);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_features, Initialize)
