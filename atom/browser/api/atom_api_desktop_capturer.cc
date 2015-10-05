// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_desktop_capturer.h"

#include "atom/common/api/atom_api_native_image.h"
#include "atom/common/node_includes.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/media/desktop_media_list.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "third_party/webrtc/modules/desktop_capture/screen_capturer.h"
#include "third_party/webrtc/modules/desktop_capture/window_capturer.h"

namespace atom {

namespace api {

namespace {
// Refresh every second.
const int kUpdatePeriod = 1000;
const int kThumbnailWidth = 150;
const int kThumbnailHeight = 150;
}  // namespace

DesktopCapturer::DesktopCapturer() {
}

DesktopCapturer::~DesktopCapturer() {
}

void DesktopCapturer::StartUpdating(const mate::Dictionary& args) {
  std::vector<std::string> sources;
  if (!args.Get("types", &sources))
    return;

  bool show_screens = false;
  bool show_windows = false;
  for (const auto& source_type : sources) {
    if (source_type == "screen")
      show_screens = true;
    else if (source_type == "window")
      show_windows = true;
  }

  if (!show_windows && !show_screens)
    return;

  webrtc::DesktopCaptureOptions options =
      webrtc::DesktopCaptureOptions::CreateDefault();

#if defined(OS_WIN)
  // On windows, desktop effects (e.g. Aero) will be disabled when the Desktop
  // capture API is active by default.
  // We keep the desktop effects in most times. Howerver, the screen still
  // fickers when the API is capturing the window due to limitation of current
  // implemetation. This is a known and wontFix issue in webrtc (see:
  // http://code.google.com/p/webrtc/issues/detail?id=3373)
  options.set_disable_effects(false);
#endif

  scoped_ptr<webrtc::ScreenCapturer> screen_capturer(
      show_screens ? webrtc::ScreenCapturer::Create(options) : nullptr);
  scoped_ptr<webrtc::WindowCapturer> window_capturer(
      show_windows ? webrtc::WindowCapturer::Create(options) : nullptr);
  media_list_.reset(new NativeDesktopMediaList(screen_capturer.Pass(),
      window_capturer.Pass()));

  int update_period = kUpdatePeriod;
  int thumbnail_width = kThumbnailWidth, thumbnail_height = kThumbnailHeight;
  args.Get("updatePeriod", &update_period);
  args.Get("thumbnailWidth", &thumbnail_width);
  args.Get("thumbnailHeight", &thumbnail_height);

  media_list_->SetUpdatePeriod(base::TimeDelta::FromMilliseconds(
        update_period));
  media_list_->SetThumbnailSize(gfx::Size(thumbnail_width, thumbnail_height));
  media_list_->StartUpdating(this);
}

void DesktopCapturer::StopUpdating() {
  media_list_.reset();
}

void DesktopCapturer::OnSourceAdded(int index) {
  EmitDesktopCapturerEvent("source-added", index, false);
}

void DesktopCapturer::OnSourceRemoved(int index) {
  EmitDesktopCapturerEvent("source-removed", index, false);
}

// Ignore this event.
void DesktopCapturer::OnSourceMoved(int old_index, int new_index) {
}

void DesktopCapturer::OnSourceNameChanged(int index) {
  EmitDesktopCapturerEvent("source-removed", index, false);
}

void DesktopCapturer::OnSourceThumbnailChanged(int index) {
  EmitDesktopCapturerEvent("source-thumbnail-changed", index, true);
}

void DesktopCapturer::OnRefreshFinished() {
  Emit("refresh-finished");
}

void DesktopCapturer::EmitDesktopCapturerEvent(
    const std::string& event_name, int index, bool with_thumbnail) {
  const DesktopMediaList::Source& source = media_list_->GetSource(index);
  content::DesktopMediaID id = source.id;
  if (!with_thumbnail)
    Emit(event_name, id.ToString(), base::UTF16ToUTF8(source.name));
  else {
    Emit(event_name, id.ToString(), base::UTF16ToUTF8(source.name),
         atom::api::NativeImage::Create(isolate(),
                                        gfx::Image(source.thumbnail)));
  }
}

mate::ObjectTemplateBuilder DesktopCapturer::GetObjectTemplateBuilder(
      v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("startUpdating", &DesktopCapturer::StartUpdating)
      .SetMethod("stopUpdating", &DesktopCapturer::StopUpdating);
}

// static
mate::Handle<DesktopCapturer> DesktopCapturer::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new DesktopCapturer);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("desktopCapturer", atom::api::DesktopCapturer::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_desktop_capturer, Initialize);
