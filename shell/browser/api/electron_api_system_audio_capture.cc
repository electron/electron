// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_system_audio_capture.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/node_includes.h"

namespace gin {

template <>
struct Converter<electron::SystemAudioCapturer::PcmFrameMeta> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const electron::SystemAudioCapturer::PcmFrameMeta& meta) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("sampleRate", meta.sample_rate);
    dict.Set("channels", static_cast<int>(meta.channels));
    dict.Set("sequence", meta.sequence);
    dict.Set("timestampMs", meta.timestamp_ms);
    dict.Set("fromSystem", meta.from_system_capture);
    dict.Set("source", std::string_view(meta.from_system_capture ? "system"
                                   : "fallback"));
    return gin::ConvertToV8(isolate, dict);
  }
};

template <>
struct Converter<electron::SystemAudioCapturer::CaptureDiagnostics> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const electron::SystemAudioCapturer::CaptureDiagnostics& diagnostics) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("permissionPreflight", diagnostics.permission_preflight);
    dict.Set("usingSystemCapture", diagnostics.using_system_capture);
    dict.Set("lastStartSystemCaptureSucceeded",
             diagnostics.last_start_system_capture_succeeded);
    dict.Set("systemFrameCallbacks", diagnostics.system_frame_callbacks);
    dict.Set("fallbackFrameCallbacks", diagnostics.fallback_frame_callbacks);
    dict.Set("queuedFrameDepth", diagnostics.queued_frame_depth);
    dict.Set("queueMaxDepth", diagnostics.queue_max_depth);
    dict.Set("queueDroppedFrames", diagnostics.queue_dropped_frames);
    dict.Set("queueUnderrunFrames", diagnostics.queue_underrun_frames);
    dict.Set("lastError", diagnostics.last_error);
    return gin::ConvertToV8(isolate, dict);
  }
};

}  // namespace gin

namespace electron::api {

gin::DeprecatedWrapperInfo SystemAudioCapture::kWrapperInfo = {
    gin::kEmbedderNativeGin};

SystemAudioCapture::SystemAudioCapture()
    : capturer_(std::make_unique<SystemAudioCapturer>()) {}

SystemAudioCapture::~SystemAudioCapture() = default;

bool SystemAudioCapture::StartHandling() {
  auto on_frame = base::BindRepeating(&SystemAudioCapture::OnFrame,
                                      base::Unretained(this));
  auto on_error = base::BindRepeating(&SystemAudioCapture::OnError,
                                      base::Unretained(this));

  if (!capturer_->Start(std::move(on_frame), std::move(on_error)))
    return false;

  Pin(electron::JavascriptEnvironment::GetIsolate());
  return true;
}

void SystemAudioCapture::StopHandling() {
  capturer_->Stop();
  Unpin();
}

SystemAudioCapturer::CaptureDiagnostics
SystemAudioCapture::GetCaptureDiagnostics() {
  return capturer_->GetCaptureDiagnostics();
}

void SystemAudioCapture::OnFrame(const std::vector<float>& frame,
                                 const SystemAudioCapturer::PcmFrameMeta& meta) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  gin_helper::CallMethod(this, "_onframe", frame, meta);
}

void SystemAudioCapture::OnError(const std::string& error) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  gin_helper::CallMethod(this, "_onerror", error);
}

// static
gin_helper::Handle<SystemAudioCapture> SystemAudioCapture::Create(
    v8::Isolate* isolate) {
  return gin_helper::CreateHandle(isolate, new SystemAudioCapture());
}

gin::ObjectTemplateBuilder SystemAudioCapture::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::DeprecatedWrappable<
             SystemAudioCapture>::GetObjectTemplateBuilder(isolate)
      .SetMethod("startHandling", &SystemAudioCapture::StartHandling)
      .SetMethod("stopHandling", &SystemAudioCapture::StopHandling)
      .SetMethod("getCaptureDiagnostics",
                 &SystemAudioCapture::GetCaptureDiagnostics);
}

const char* SystemAudioCapture::GetTypeName() {
  return "SystemAudioCapture";
}

}  // namespace electron::api

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("createSystemAudioCapture",
                 &electron::api::SystemAudioCapture::Create);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_system_audio_capture,
                                  Initialize)
