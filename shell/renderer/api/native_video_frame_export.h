// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license.
#ifndef ELECTRON_SHELL_RENDERER_API_NATIVE_VIDEO_FRAME_EXPORT_H_
#define ELECTRON_SHELL_RENDERER_API_NATIVE_VIDEO_FRAME_EXPORT_H_
#include "v8/include/v8.h"
namespace electron {
void InstallNativeVideoFrameExport(v8::Isolate* isolate,
                                   v8::Local<v8::Object> exports);
}
#endif  // ELECTRON_SHELL_RENDERER_API_NATIVE_VIDEO_FRAME_EXPORT_H_
