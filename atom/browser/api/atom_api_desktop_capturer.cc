// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_desktop_capturer.h"

#include "atom/common/api/atom_api_native_image.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/node_includes.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/media/desktop_media_list.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "third_party/webrtc/modules/desktop_capture/screen_capturer.h"
#include "third_party/webrtc/modules/desktop_capture/window_capturer.h"

namespace mate {

template<>
struct Converter<DesktopMediaList::Source> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const DesktopMediaList::Source& source) {
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    content::DesktopMediaID id = source.id;
    dict.Set("name", base::UTF16ToUTF8(source.name));
    dict.Set("id", id.ToString());
    dict.Set("thumbnail",
        atom::api::NativeImage::Create(isolate, gfx::Image(source.thumbnail)));
    return ConvertToV8(isolate, dict);
  }
};

}  // namespace mate

namespace atom {

namespace api {

namespace {
// The wrapDesktopCapturer funtion which is implemented in JavaScript
using WrapDesktopCapturerCallback = base::Callback<void(v8::Local<v8::Value>)>;
WrapDesktopCapturerCallback g_wrap_desktop_capturer;

const int kThumbnailWidth = 150;
const int kThumbnailHeight = 150;
}  // namespace

DesktopCapturer::DesktopCapturer(bool show_screens, bool show_windows) {
  scoped_ptr<webrtc::ScreenCapturer> screen_capturer(
      show_screens ? webrtc::ScreenCapturer::Create() : nullptr);
  scoped_ptr<webrtc::WindowCapturer> window_capturer(
      show_windows ? webrtc::WindowCapturer::Create() : nullptr);
  media_list_.reset(new NativeDesktopMediaList(screen_capturer.Pass(),
      window_capturer.Pass()));
  media_list_->SetThumbnailSize(gfx::Size(kThumbnailWidth, kThumbnailHeight));
  media_list_->StartUpdating(this);
}

DesktopCapturer::~DesktopCapturer() {
}

const DesktopMediaList::Source& DesktopCapturer::GetSource(int index) {
  return media_list_->GetSource(index);
}

void DesktopCapturer::OnSourceAdded(int index) {
  Emit("source-added", index);
}

void DesktopCapturer::OnSourceRemoved(int index) {
  Emit("source-removed", index);
}

void DesktopCapturer::OnSourceMoved(int old_index, int new_index) {
  Emit("source-moved", old_index, new_index);
}

void DesktopCapturer::OnSourceNameChanged(int index) {
  Emit("source-name-changed", index);
}

void DesktopCapturer::OnSourceThumbnailChanged(int index) {
  Emit("source-thumbnail-changed", index);
}

mate::ObjectTemplateBuilder DesktopCapturer::GetObjectTemplateBuilder(
      v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("getSource", &DesktopCapturer::GetSource);
}

void SetWrapDesktopCapturer(const WrapDesktopCapturerCallback& callback) {
  g_wrap_desktop_capturer = callback;
}

void ClearWrapDesktopCapturer() {
  g_wrap_desktop_capturer.Reset();
}

// static
mate::Handle<DesktopCapturer> DesktopCapturer::Create(v8::Isolate* isolate,
    bool show_screens, bool show_windows) {
  auto handle = mate::CreateHandle(isolate,
      new DesktopCapturer(show_screens, show_windows));
  g_wrap_desktop_capturer.Run(handle.ToV8());
  return handle;
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("_setWrapDesktopCapturer", &atom::api::SetWrapDesktopCapturer);
  dict.SetMethod("_clearWrapDesktopCapturer",
                 &atom::api::ClearWrapDesktopCapturer);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_desktop_capturer, Initialize);
