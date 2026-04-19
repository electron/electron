// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SYSTEM_AUDIO_CAPTURE_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SYSTEM_AUDIO_CAPTURE_H_

#include <memory>

#include "shell/browser/media/system_audio_capturer.h"
#include "shell/common/gin_helper/pinnable.h"
#include "shell/common/gin_helper/wrappable.h"

namespace gin_helper {
template <typename T>
class Handle;
}  // namespace gin_helper

namespace electron::api {

class SystemAudioCapture final
    : public gin_helper::DeprecatedWrappable<SystemAudioCapture>,
      public gin_helper::Pinnable<SystemAudioCapture> {
 public:
  static gin_helper::Handle<SystemAudioCapture> Create(v8::Isolate* isolate);

  bool StartHandling();
  void StopHandling();
  SystemAudioCapturer::CaptureDiagnostics GetCaptureDiagnostics();

  // gin_helper::Wrappable
  static gin::DeprecatedWrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  SystemAudioCapture(const SystemAudioCapture&) = delete;
  SystemAudioCapture& operator=(const SystemAudioCapture&) = delete;

 protected:
  SystemAudioCapture();
  ~SystemAudioCapture() override;

 private:
  void OnFrame(const std::vector<float>& frame,
               const SystemAudioCapturer::PcmFrameMeta& meta);
  void OnError(const std::string& error);

  std::unique_ptr<SystemAudioCapturer> capturer_;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SYSTEM_AUDIO_CAPTURE_H_
