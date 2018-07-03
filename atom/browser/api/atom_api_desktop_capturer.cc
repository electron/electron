// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_desktop_capturer.h"

#include <vector>

using base::PlatformThreadRef;

#include "atom/common/api/atom_api_native_image.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/media/desktop_media_list.h"
#include "content/public/browser/browser_thread.h"
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
    return ConvertToV8(isolate, dict);
  }
};

}  // namespace mate

namespace {

void EmitFinished(
    const std::vector<atom::api::DesktopCapturer::Source>& sources,
    atom::api::DesktopCapturer* cap) {
  cap->Emit("finished", sources);
}

void StartHandlingTask(bool capture_window,
                       bool capture_screen,
                       const gfx::Size& thumbnail_size,
                       atom::api::DesktopCapturer* cap) {
#if defined(OS_WIN)
  if (content::desktop_capture::CreateDesktopCaptureOptions()
      .allow_directx_capturer()) {
    // DxgiDuplicatorController should be alive in this scope according to
    // screen_capturer_win.cc.
    auto duplicator = webrtc::DxgiDuplicatorController::Instance();
    cap->using_directx_capturer_ =
        webrtc::ScreenCapturerWinDirectx::IsSupported();
  }
#endif  // defined(OS_WIN)
  std::unique_ptr<webrtc::DesktopCapturer> screen_capturer(
      capture_screen ? content::desktop_capture::CreateScreenCapturer()
                     : nullptr);
  std::unique_ptr<webrtc::DesktopCapturer> window_capturer(
      capture_window ? content::desktop_capture::CreateWindowCapturer()
                     : nullptr);
  cap->media_list_.reset(new NativeDesktopMediaList(
      std::move(screen_capturer), std::move(window_capturer)));

  cap->media_list_->SetThumbnailSize(thumbnail_size);
  cap->media_list_->StartUpdating(cap);
}

void OnRefreshFinishedTask(atom::api::DesktopCapturer* cap) {
  const auto media_list_sources = cap->media_list_->GetSources();
  std::vector<atom::api::DesktopCapturer::Source> sources;
  for (const auto& media_list_source : media_list_sources) {
    sources.emplace_back(
        atom::api::DesktopCapturer::Source{media_list_source, std::string()});
  }

#if defined(OS_WIN)
  // Gather the same unique screen IDs used by the electron.screen API in order
  // to provide an association between it and desktopCapturer/getUserMedia.
  // This is only required when using the DirectX capturer, otherwise the IDs
  // across the APIs already match.
  if (cap->using_directx_capturer_) {
    std::vector<std::string> device_names;
    // Crucially, this list of device names will be in the same order as
    // |media_list_sources|.
    webrtc::DxgiDuplicatorController::Instance()->GetDeviceNames(&device_names);
    int device_name_index = 0;
    for (auto& source : sources) {
      if (source.media_list_source.id.type ==
          content::DesktopMediaID::TYPE_SCREEN) {
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
  }
#elif defined(OS_MACOSX)
  // On Mac, the IDs across the APIs match.
  for (auto& source : sources) {
    if (source.media_list_source.id.type ==
        content::DesktopMediaID::TYPE_SCREEN) {
      source.display_id = base::Int64ToString(source.media_list_source.id.id);
    }
  }
#endif  // defined(OS_WIN)
  // TODO(ajmacd): Add Linux support. The IDs across APIs differ but Chrome only
  // supports capturing the entire desktop on Linux. Revisit this if individual
  // screen support is added.

  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   base::Bind(EmitFinished, sources, cap));
}

}  // namespace

namespace atom {

namespace api {

DesktopCapturer::DesktopCapturer(v8::Isolate* isolate) {
  Init(isolate);
  capture_thread_ = base::CreateSequencedTaskRunnerWithTraits(
      {base::WithBaseSyncPrimitives(), base::MayBlock(),
       base::TaskPriority::USER_VISIBLE});
}

DesktopCapturer::~DesktopCapturer() {}

void DesktopCapturer::StartHandling(bool capture_window,
                                    bool capture_screen,
                                    const gfx::Size& thumbnail_size) {
  capture_thread_->PostTask(
      FROM_HERE, base::BindOnce(StartHandlingTask, capture_window,
                                capture_screen, thumbnail_size, this));
}

void DesktopCapturer::OnSourceAdded(int index) {}

void DesktopCapturer::OnSourceRemoved(int index) {}

void DesktopCapturer::OnSourceMoved(int old_index, int new_index) {}

void DesktopCapturer::OnSourceNameChanged(int index) {}

void DesktopCapturer::OnSourceThumbnailChanged(int index) {}

bool DesktopCapturer::OnRefreshFinished() {
  capture_thread_->PostTask(FROM_HERE,
                            base::BindOnce(OnRefreshFinishedTask, this));
  return false;
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
