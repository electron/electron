// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_desktop_capturer.h"

#include <memory>
#include <utility>
#include <vector>

#include "atom/common/api/atom_api_native_image.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "chrome/browser/media/webrtc/window_icon_util.h"
#include "content/public/browser/desktop_capture.h"
#include "native_mate/dictionary.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capturer.h"

#if defined(OS_WIN)
#include "third_party/webrtc/modules/desktop_capture/win/dxgi_duplicator_controller.h"
#include "third_party/webrtc/modules/desktop_capture/win/screen_capturer_win_directx.h"
#include "ui/display/win/display_info.h"
#endif  // defined(OS_WIN)

#include "atom/common/node_includes.h"

namespace mate {

template <>
struct Converter<atom::api::DesktopCapturer::Source> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const atom::api::DesktopCapturer::Source& source) {
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    content::DesktopMediaID id = source.media_list_source.id;
    dict.Set("name", base::UTF16ToUTF8(source.media_list_source.name));
    dict.Set("id", id.ToString());
    dict.Set("thumbnail",
             atom::api::NativeImage::Create(
                 isolate, gfx::Image(source.media_list_source.thumbnail)));
    dict.Set("display_id", source.display_id);
    if (source.fetch_icon) {
      dict.Set(
          "appIcon",
          atom::api::NativeImage::Create(
              isolate, gfx::Image(GetWindowIcon(source.media_list_source.id))));
    }
    return ConvertToV8(isolate, dict);
  }
};

}  // namespace mate

namespace atom {

namespace api {

DesktopCapturer::DesktopCapturer(v8::Isolate* isolate) {
  Init(isolate);
}

DesktopCapturer::~DesktopCapturer() {}

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
    // Remove this once
    // https://bugs.chromium.org/p/chromium/issues/detail?id=795340 is fixed.
    base::ScopedAllowBaseSyncPrimitivesForTesting
        scoped_allow_base_sync_primitives;
    // Initialize the source list.
    // Apply the new thumbnail size and restart capture.
    if (capture_window) {
      window_capturer_.reset(new NativeDesktopMediaList(
          content::DesktopMediaID::TYPE_WINDOW,
          content::desktop_capture::CreateWindowCapturer()));
      window_capturer_->SetThumbnailSize(thumbnail_size);
      window_capturer_->AddObserver(this);
      window_capturer_->StartUpdating();
    }

    if (capture_screen) {
      screen_capturer_.reset(new NativeDesktopMediaList(
          content::DesktopMediaID::TYPE_SCREEN,
          content::desktop_capture::CreateScreenCapturer()));
      screen_capturer_->SetThumbnailSize(thumbnail_size);
      screen_capturer_->AddObserver(this);
      screen_capturer_->StartUpdating();
    }
  }
}

void DesktopCapturer::OnSourceAdded(DesktopMediaList* list, int index) {}

void DesktopCapturer::OnSourceRemoved(DesktopMediaList* list, int index) {}

void DesktopCapturer::OnSourceMoved(DesktopMediaList* list,
                                    int old_index,
                                    int new_index) {}

void DesktopCapturer::OnSourceNameChanged(DesktopMediaList* list, int index) {}

void DesktopCapturer::OnSourceThumbnailChanged(DesktopMediaList* list,
                                               int index) {}

void DesktopCapturer::OnSourceUnchanged(DesktopMediaList* list) {
  UpdateSourcesList(list);
}

bool DesktopCapturer::ShouldScheduleNextRefresh(DesktopMediaList* list) {
  UpdateSourcesList(list);
  return false;
}

void DesktopCapturer::UpdateSourcesList(DesktopMediaList* list) {
  if (capture_window_ &&
      list->GetMediaListType() == content::DesktopMediaID::TYPE_WINDOW) {
    capture_window_ = false;
    const auto& media_list_sources = list->GetSources();
    std::vector<DesktopCapturer::Source> window_sources;
    window_sources.reserve(media_list_sources.size());
    for (const auto& media_list_source : media_list_sources) {
      window_sources.emplace_back(DesktopCapturer::Source{
          media_list_source, std::string(), fetch_window_icons_});
    }
    std::move(window_sources.begin(), window_sources.end(),
              std::back_inserter(captured_sources_));
  }

  if (capture_screen_ &&
      list->GetMediaListType() == content::DesktopMediaID::TYPE_SCREEN) {
    capture_screen_ = false;
    const auto& media_list_sources = list->GetSources();
    std::vector<DesktopCapturer::Source> screen_sources;
    screen_sources.reserve(media_list_sources.size());
    for (const auto& media_list_source : media_list_sources) {
      screen_sources.emplace_back(
          DesktopCapturer::Source{media_list_source, std::string()});
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
      webrtc::DxgiDuplicatorController::Instance()->GetDeviceNames(
          &device_names);
      int device_name_index = 0;
      for (auto& source : screen_sources) {
        const auto& device_name = device_names[device_name_index++];
        std::wstring wide_device_name;
        base::UTF8ToWide(device_name.c_str(), device_name.size(),
                         &wide_device_name);
        const int64_t device_id =
            display::win::DisplayInfo::DeviceIdFromDeviceName(
                wide_device_name.c_str());
        source.display_id = base::Int64ToString(device_id);
      }
    }
#elif defined(OS_MACOSX)
    // On Mac, the IDs across the APIs match.
    for (auto& source : screen_sources) {
      source.display_id = base::Int64ToString(source.media_list_source.id.id);
    }
#endif  // defined(OS_WIN)
    // TODO(ajmacd): Add Linux support. The IDs across APIs differ but Chrome
    // only supports capturing the entire desktop on Linux. Revisit this if
    // individual screen support is added.
    std::move(screen_sources.begin(), screen_sources.end(),
              std::back_inserter(captured_sources_));
  }

  if (!capture_window_ && !capture_screen_)
    Emit("finished", captured_sources_, fetch_window_icons_);
}

// static
mate::Handle<DesktopCapturer> DesktopCapturer::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new DesktopCapturer(isolate));
}

// static
void DesktopCapturer::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "DesktopCapturer"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("startHandling", &DesktopCapturer::StartHandling);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("desktopCapturer", atom::api::DesktopCapturer::Create(isolate));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_desktop_capturer, Initialize);
