// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_desktop_capturer.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "chrome/browser/media/webrtc/window_icon_util.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/desktop_capture.h"
#include "content/public/browser/media_device_request.h"
#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_includes.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capturer.h"

#if defined(OS_WIN)
#include "third_party/webrtc/modules/desktop_capture/win/dxgi_duplicator_controller.h"
#include "third_party/webrtc/modules/desktop_capture/win/screen_capturer_win_directx.h"
#include "ui/display/win/display_info.h"
#endif  // defined(OS_WIN)

namespace gin {

template <>
struct Converter<electron::api::DesktopCapturer::Source> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const electron::api::DesktopCapturer::Source& source) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    content::DesktopMediaID id = source.media_list_source.id;
    dict.Set("name", base::UTF16ToUTF8(source.media_list_source.name));
    dict.Set("id", id.ToString());
    dict.Set("thumbnail",
             electron::api::NativeImage::Create(
                 isolate, gfx::Image(source.media_list_source.thumbnail)));
    dict.Set("display_id", source.display_id);
    if (source.fetch_icon) {
      dict.Set(
          "appIcon",
          electron::api::NativeImage::Create(
              isolate, gfx::Image(GetWindowIcon(source.media_list_source.id))));
    } else {
      dict.Set("appIcon", nullptr);
    }
    return ConvertToV8(isolate, dict);
  }
};

}  // namespace gin

namespace electron {

namespace api {

gin::WrapperInfo DesktopCapturer::kWrapperInfo = {gin::kEmbedderNativeGin};

DesktopCapturer::DesktopCapturer(v8::Isolate* isolate) {}

DesktopCapturer::~DesktopCapturer() = default;

void DesktopCapturer::StartHandling(bool capture_window,
                                    bool capture_screen,
                                    const gfx::Size& thumbnail_size,
                                    bool fetch_window_icons) {
  fetch_window_icons_ = fetch_window_icons;
#if defined(OS_WIN)
  if (content::desktop_capture::CreateDesktopCaptureOptions()
          .allow_directx_capturer()) {
    // DxgiDuplicatorController should be alive in this scope according to
    // screen_capturer_win.cc.
    auto duplicator = webrtc::DxgiDuplicatorController::Instance();
    using_directx_capturer_ = webrtc::ScreenCapturerWinDirectx::IsSupported();
  }
#endif  // defined(OS_WIN)

  // clear any existing captured sources.
  captured_sources_.clear();

  // Start listening for captured sources.
  capture_window_ = capture_window;
  capture_screen_ = capture_screen;

  {
    // Initialize the source list.
    // Apply the new thumbnail size and restart capture.
    if (capture_window) {
      window_capturer_ = std::make_unique<NativeDesktopMediaList>(
          DesktopMediaList::Type::kWindow,
          content::desktop_capture::CreateWindowCapturer());
      window_capturer_->SetThumbnailSize(thumbnail_size);
      window_capturer_->Update(
          base::BindOnce(&DesktopCapturer::UpdateSourcesList,
                         weak_ptr_factory_.GetWeakPtr(),
                         window_capturer_.get()),
          /* refresh_thumbnails = */ true);
    }

    if (capture_screen) {
      screen_capturer_ = std::make_unique<NativeDesktopMediaList>(
          DesktopMediaList::Type::kScreen,
          content::desktop_capture::CreateScreenCapturer());
      screen_capturer_->SetThumbnailSize(thumbnail_size);
      screen_capturer_->Update(
          base::BindOnce(&DesktopCapturer::UpdateSourcesList,
                         weak_ptr_factory_.GetWeakPtr(),
                         screen_capturer_.get()),
          /* refresh_thumbnails = */ true);
    }
  }
}

void DesktopCapturer::UpdateSourcesList(DesktopMediaList* list) {
  if (capture_window_ &&
      list->GetMediaListType() == DesktopMediaList::Type::kWindow) {
    capture_window_ = false;
    std::vector<DesktopCapturer::Source> window_sources;
    window_sources.reserve(list->GetSourceCount());
    for (int i = 0; i < list->GetSourceCount(); i++) {
      window_sources.emplace_back(DesktopCapturer::Source{
          list->GetSource(i), std::string(), fetch_window_icons_});
    }
    std::move(window_sources.begin(), window_sources.end(),
              std::back_inserter(captured_sources_));
  }

  if (capture_screen_ &&
      list->GetMediaListType() == DesktopMediaList::Type::kScreen) {
    capture_screen_ = false;
    std::vector<DesktopCapturer::Source> screen_sources;
    screen_sources.reserve(list->GetSourceCount());
    for (int i = 0; i < list->GetSourceCount(); i++) {
      screen_sources.emplace_back(
          DesktopCapturer::Source{list->GetSource(i), std::string()});
    }
#if defined(OS_WIN)
    // Gather the same unique screen IDs used by the electron.screen API in
    // order to provide an association between it and
    // desktopCapturer/getUserMedia. This is only required when using the
    // DirectX capturer, otherwise the IDs across the APIs already match.
    if (using_directx_capturer_) {
      std::vector<std::string> device_names;
      // Crucially, this list of device names will be in the same order as
      // |media_list_sources|.
      if (!webrtc::DxgiDuplicatorController::Instance()->GetDeviceNames(
              &device_names)) {
        v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
        v8::Locker locker(isolate);
        v8::HandleScope scope(isolate);
        gin_helper::CallMethod(this, "_onerror", "Failed to get sources.");

        Unpin();

        return;
      }

      int device_name_index = 0;
      for (auto& source : screen_sources) {
        const auto& device_name = device_names[device_name_index++];
        std::wstring wide_device_name;
        base::UTF8ToWide(device_name.c_str(), device_name.size(),
                         &wide_device_name);
        const int64_t device_id =
            display::win::DisplayInfo::DeviceIdFromDeviceName(
                wide_device_name.c_str());
        source.display_id = base::NumberToString(device_id);
      }
    }
#elif defined(OS_MAC)
    // On Mac, the IDs across the APIs match.
    for (auto& source : screen_sources) {
      source.display_id = base::NumberToString(source.media_list_source.id.id);
    }
#endif  // defined(OS_WIN)
    // TODO(ajmacd): Add Linux support. The IDs across APIs differ but Chrome
    // only supports capturing the entire desktop on Linux. Revisit this if
    // individual screen support is added.
    std::move(screen_sources.begin(), screen_sources.end(),
              std::back_inserter(captured_sources_));
  }

  if (!capture_window_ && !capture_screen_) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::Locker locker(isolate);
    v8::HandleScope scope(isolate);
    gin_helper::CallMethod(this, "_onfinished", captured_sources_);

    Unpin();
  }
}

// static
gin::Handle<DesktopCapturer> DesktopCapturer::Create(v8::Isolate* isolate) {
  auto handle = gin::CreateHandle(isolate, new DesktopCapturer(isolate));

  // Keep reference alive until capturing has finished.
  handle->Pin(isolate);

  return handle;
}

gin::ObjectTemplateBuilder DesktopCapturer::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<DesktopCapturer>::GetObjectTemplateBuilder(isolate)
      .SetMethod("startHandling", &DesktopCapturer::StartHandling);
}

const char* DesktopCapturer::GetTypeName() {
  return "DesktopCapturer";
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createDesktopCapturer",
                 &electron::api::DesktopCapturer::Create);
  dict.SetMethod("setSkipCursor", &content::MediaDeviceRequest::SetSkipCursor);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_desktop_capturer, Initialize)
