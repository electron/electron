// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_desktop_capturer.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "chrome/browser/media/webrtc/desktop_capturer_wrapper.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "chrome/browser/media/webrtc/desktop_media_list_observer.h"
#include "chrome/browser/media/webrtc/native_desktop_media_list.h"
#include "chrome/browser/media/webrtc/thumbnail_capturer_mac.h"
#include "chrome/browser/media/webrtc/window_icon_util.h"
#include "content/public/browser/desktop_capture.h"
#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/node_includes.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capturer.h"
#include "ui/base/ozone_buildflags.h"

#if BUILDFLAG(IS_WIN)
#include "third_party/webrtc/modules/desktop_capture/win/dxgi_duplicator_controller.h"
#include "third_party/webrtc/modules/desktop_capture/win/screen_capturer_win_directx.h"
#include "ui/display/win/display_info.h"
#elif BUILDFLAG(SUPPORTS_OZONE_X11)
#include "base/logging.h"
#include "ui/base/x/x11_display_util.h"
#include "ui/base/x/x11_util.h"
#include "ui/display/util/edid_parser.h"  // nogncheck
#include "ui/gfx/x/atom_cache.h"
#include "ui/gfx/x/randr.h"
#endif

#if BUILDFLAG(IS_MAC)
#include "base/strings/string_number_conversions.h"
#include "ui/base/cocoa/permissions_utils.h"
#endif

namespace {
#if BUILDFLAG(IS_LINUX)
// Private function in ui/base/x/x11_display_util.cc
base::flat_map<x11::RandR::Output, int> GetMonitors(
    std::pair<uint32_t, uint32_t> version,
    x11::RandR* randr,
    x11::Window window) {
  base::flat_map<x11::RandR::Output, int> output_to_monitor;
  if (version >= std::pair<uint32_t, uint32_t>{1, 5}) {
    if (auto reply = randr->GetMonitors({window}).Sync()) {
      for (size_t monitor = 0; monitor < reply->monitors.size(); monitor++) {
        for (x11::RandR::Output output : reply->monitors[monitor].outputs)
          output_to_monitor[output] = monitor;
      }
    }
  }
  return output_to_monitor;
}
// Get the EDID data from the |output| and stores to |edid|.
// Private function in ui/base/x/x11_display_util.cc
std::vector<uint8_t> GetEDIDProperty(x11::RandR* randr,
                                     x11::RandR::Output output) {
  constexpr const char kRandrEdidProperty[] = "EDID";
  auto future = randr->GetOutputProperty(x11::RandR::GetOutputPropertyRequest{
      .output = output,
      .property = x11::GetAtom(kRandrEdidProperty),
      .long_length = 128});
  auto response = future.Sync();
  std::vector<uint8_t> edid;
  if (response && response->format == 8 && response->type != x11::Atom::None)
    edid = std::move(response->data);
  return edid;
}

// Find the mapping from monitor name atom to the display identifier
// that the screen API uses. Based on the logic in BuildDisplaysFromXRandRInfo
// in ui/base/x/x11_display_util.cc
base::flat_map<int32_t, uint32_t> MonitorAtomIdToDisplayId() {
  auto* connection = x11::Connection::Get();
  auto& randr = connection->randr();
  auto x_root_window = ui::GetX11RootWindow();

  base::flat_map<int32_t, uint32_t> monitor_atom_to_display;

  auto resources = randr.GetScreenResourcesCurrent({x_root_window}).Sync();
  if (!resources) {
    LOG(ERROR) << "XRandR returned no displays; don't know how to map ids";
    return monitor_atom_to_display;
  }

  const auto output_to_monitor =
      GetMonitors(connection->randr_version(), &randr, x_root_window);
  auto monitors_reply = randr.GetMonitors({x_root_window}).Sync();

  for (size_t i = 0; i < resources->outputs.size(); i++) {
    x11::RandR::Output output_id = resources->outputs[i];
    auto output_info =
        randr.GetOutputInfo({output_id, resources->config_timestamp}).Sync();
    if (!output_info)
      continue;

    if (output_info->connection != x11::RandR::RandRConnection::Connected)
      continue;

    if (output_info->crtc == static_cast<x11::RandR::Crtc>(0))
      continue;

    auto crtc =
        randr.GetCrtcInfo({output_info->crtc, resources->config_timestamp})
            .Sync();
    if (!crtc)
      continue;
    display::EdidParser edid_parser(
        GetEDIDProperty(&randr, static_cast<x11::RandR::Output>(output_id)));
    auto output_32 = static_cast<uint32_t>(output_id);
    int64_t display_id =
        output_32 > 0xff ? 0 : edid_parser.GetIndexBasedDisplayId(output_32);
    // It isn't ideal, but if we can't parse the EDID data, fall back on the
    // display number.
    if (!display_id)
      display_id = i;

    // Find the mapping between output identifier and the monitor name atom
    // Note this isn't the atom string, but the numeric atom identifier,
    // since this is what the WebRTC system uses as the display identifier
    auto output_monitor_iter = output_to_monitor.find(output_id);
    if (output_monitor_iter != output_to_monitor.end()) {
      x11::Atom atom =
          monitors_reply->monitors[output_monitor_iter->second].name;
      monitor_atom_to_display[static_cast<int32_t>(atom)] = display_id;
    }
  }

  return monitor_atom_to_display;
}
#endif

std::unique_ptr<ThumbnailCapturer> MakeWindowCapturer() {
#if BUILDFLAG(IS_MAC)
  if (ShouldUseThumbnailCapturerMac(DesktopMediaList::Type::kWindow)) {
    return CreateThumbnailCapturerMac(DesktopMediaList::Type::kWindow,
                                      /*web_contents=*/nullptr);
  }
#endif  // BUILDFLAG(IS_MAC)

  std::unique_ptr<webrtc::DesktopCapturer> window_capturer =
      content::desktop_capture::CreateWindowCapturer(
          content::desktop_capture::CreateDesktopCaptureOptions());
  return window_capturer ? std::make_unique<DesktopCapturerWrapper>(
                               std::move(window_capturer))
                         : nullptr;
}

std::unique_ptr<ThumbnailCapturer> MakeScreenCapturer() {
#if BUILDFLAG(IS_MAC)
  if (ShouldUseThumbnailCapturerMac(DesktopMediaList::Type::kScreen)) {
    return CreateThumbnailCapturerMac(DesktopMediaList::Type::kScreen,
                                      /*web_contents=*/nullptr);
  }
#endif  // BUILDFLAG(IS_MAC)

  std::unique_ptr<webrtc::DesktopCapturer> screen_capturer =
      content::desktop_capture::CreateScreenCapturer(
          content::desktop_capture::CreateDesktopCaptureOptions(),
          /*for_snapshot=*/false);
  return screen_capturer ? std::make_unique<DesktopCapturerWrapper>(
                               std::move(screen_capturer))
                         : nullptr;
}

#if BUILDFLAG(IS_WIN)
BOOL CALLBACK EnumDisplayMonitorsCallback(HMONITOR monitor,
                                          HDC hdc,
                                          LPRECT rect,
                                          LPARAM data) {
  reinterpret_cast<std::vector<HMONITOR>*>(data)->push_back(monitor);
  return TRUE;
}
#endif

// Give native capturers a few refresh/capture cycles to populate the list
// before returning the current snapshot. This preserves the old behavior of
// making progress even when some sources never produce thumbnails.
constexpr base::TimeDelta kDesktopCapturerReadyTimeout = base::Seconds{3};

}  // namespace

namespace gin {

template <>
struct Converter<electron::api::DesktopCapturer::Source> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const electron::api::DesktopCapturer::Source& source) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
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

namespace electron::api {

gin::DeprecatedWrapperInfo DesktopCapturer::kWrapperInfo = {
    gin::kEmbedderNativeGin};

// Observer that forwards DesktopMediaListObserver events back to
// the owning DesktopCapturer, tagging them with the list type so
// the capturer knows which list fired.
class DesktopCapturer::ListObserver : public DesktopMediaListObserver {
 public:
  ListObserver(DesktopCapturer* capturer,
               DesktopMediaList* list,
               bool need_thumbnails)
      : capturer_{capturer}, list_{list}, need_thumbnails_{need_thumbnails} {}
  ~ListObserver() override = default;

  [[nodiscard]] bool IsReady() const {
    if (!has_sources_)
      return false;

    if (!need_thumbnails_)
      return true;

    if (list_->GetSourceCount() == 0)
      return true;

    // are all the tumbnails ready?
    for (int i = 0; i < list_->GetSourceCount(); ++i) {
      if (list_->GetSource(i).thumbnail.isNull())
        return false;
    }
    return true;
  }

 private:
  // Post OnListReady to the message loop when sources & thumbnails are done.
  void MaybeNotifyReady() {
    if (!IsReady() || notified_)
      return;
    notified_ = true;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&DesktopCapturer::OnListReady,
                       capturer_->weak_ptr_factory_.GetWeakPtr(), list_.get()));
  }

  // DesktopMediaListObserver:
  void OnSourceAdded(int index) override {
    has_sources_ = true;
    MaybeNotifyReady();
  }
  void OnSourceRemoved(int index) override { MaybeNotifyReady(); }
  void OnSourceMoved(int old_index, int new_index) override {}
  void OnSourceNameChanged(int index) override {}
  void OnSourceThumbnailChanged(int index) override { MaybeNotifyReady(); }
  void OnSourcePreviewChanged(size_t index) override {}
  void OnDelegatedSourceListSelection() override {
    // For delegated source lists (e.g. PipeWire), the selection event means
    // the user picked a source. Treat as ready once we also have a thumbnail.
    has_sources_ = true;
    MaybeNotifyReady();
  }
  void OnDelegatedSourceListDismissed() override {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&DesktopCapturer::HandleFailure,
                                  capturer_->weak_ptr_factory_.GetWeakPtr()));
  }

  raw_ptr<DesktopCapturer> capturer_;
  raw_ptr<DesktopMediaList> list_;
  bool need_thumbnails_ = false;
  bool has_sources_ = false;
  bool notified_ = false;
};

DesktopCapturer::DesktopCapturer() = default;

DesktopCapturer::~DesktopCapturer() = default;

void DesktopCapturer::FinalizeList(std::unique_ptr<ListObserver>& observer,
                                   std::unique_ptr<DesktopMediaList>& list) {
  DCHECK(observer);
  DCHECK(list);

  CollectSourcesFrom(list.get());
  if (finished_)
    return;

  list.reset();
  observer.reset();

  if (--pending_lists_ == 0)
    HandleSuccess();
}

void DesktopCapturer::StartHandling(bool capture_window,
                                    bool capture_screen,
                                    const gfx::Size& thumbnail_size,
                                    bool fetch_window_icons) {
  fetch_window_icons_ = fetch_window_icons;
#if BUILDFLAG(IS_WIN)
  if (content::desktop_capture::CreateDesktopCaptureOptions()
          .allow_directx_capturer()) {
    // DxgiDuplicatorController should be alive in this scope according to
    // screen_capturer_win.cc.
    auto duplicator = webrtc::DxgiDuplicatorController::Instance();
    using_directx_capturer_ = webrtc::ScreenCapturerWinDirectx::IsSupported();
  }
#endif  // BUILDFLAG(IS_WIN)

  // clear any existing captured sources.
  captured_sources_.clear();
  finished_ = false;
  pending_lists_ = 0;
  deadline_.Stop();

#if BUILDFLAG(IS_MAC)
  if (!ui::TryPromptUserForScreenCapture()) {
    HandleFailure();
    return;
  }
#endif

  const bool need_thumbnails = !thumbnail_size.IsEmpty();
  bool need_screen = capture_screen;

  if (capture_window) {
    // Some capturers like PipeWire support a single capturer for both screens
    // and windows. Use it if possible, treating both as window capture.
    std::unique_ptr<ThumbnailCapturer> capturer;
    bool use_generic = false;
    if (capture_screen) {
      auto desktop_capturer = webrtc::DesktopCapturer::CreateGenericCapturer(
          content::desktop_capture::CreateDesktopCaptureOptions());
      auto wrapper = desktop_capturer
                         ? std::make_unique<DesktopCapturerWrapper>(
                               std::move(desktop_capturer))
                         : nullptr;
      if (wrapper && wrapper->GetDelegatedSourceListController()) {
        capturer = std::move(wrapper);
        use_generic = true;
        // Generic capturer handles both types as window capture.
        need_screen = false;
      }
    }
    if (!capturer)
      capturer = MakeWindowCapturer();

    if (capturer) {
      window_capturer_ = std::make_unique<NativeDesktopMediaList>(
          DesktopMediaList::Type::kWindow, std::move(capturer),
          /* add_current_process_windows = */ true,
          /* auto_show_delegated_source_list = */ use_generic);
      window_capturer_->SetThumbnailSize(thumbnail_size);
      window_observer_ = std::make_unique<ListObserver>(
          this, window_capturer_.get(), need_thumbnails);
      window_capturer_->StartUpdating(window_observer_.get());
      pending_lists_++;
    }
  }

  if (need_screen) {
    auto capturer = MakeScreenCapturer();
    if (capturer) {
      screen_capturer_ = std::make_unique<NativeDesktopMediaList>(
          DesktopMediaList::Type::kScreen, std::move(capturer));
      screen_capturer_->SetThumbnailSize(thumbnail_size);
      screen_observer_ = std::make_unique<ListObserver>(
          this, screen_capturer_.get(), need_thumbnails);
      screen_capturer_->StartUpdating(screen_observer_.get());
      pending_lists_++;
    }
  }

  // If no capturers were created (e.g. all factories returned nullptr),
  // resolve immediately with an empty source list.
  if (pending_lists_ == 0) {
    HandleSuccess();
    return;
  }

  deadline_.Start(FROM_HERE, kDesktopCapturerReadyTimeout,
                  base::BindOnce(&DesktopCapturer::OnReadyTimeout,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void DesktopCapturer::OnListReady(DesktopMediaList* list) {
  if (finished_)
    return;

  if (list == window_capturer_.get()) {
    FinalizeList(window_observer_, window_capturer_);
  } else if (list == screen_capturer_.get()) {
    FinalizeList(screen_observer_, screen_capturer_);
  } else {
    NOTREACHED();
  }
}

void DesktopCapturer::OnReadyTimeout() {
  if (finished_)
    return;

  if (window_capturer_)
    FinalizeList(window_observer_, window_capturer_);

  if (finished_)
    return;

  if (screen_capturer_)
    FinalizeList(screen_observer_, screen_capturer_);

  DCHECK(finished_ || pending_lists_ > 0);
}

void DesktopCapturer::CollectSourcesFrom(DesktopMediaList* list) {
  const auto type = list->GetMediaListType();

  if (type == DesktopMediaList::Type::kWindow) {
    std::vector<DesktopCapturer::Source> window_sources;
    window_sources.reserve(list->GetSourceCount());
    for (int i = 0; i < list->GetSourceCount(); i++) {
      window_sources.emplace_back(list->GetSource(i), std::string(),
                                  fetch_window_icons_);
    }
    std::move(window_sources.begin(), window_sources.end(),
              std::back_inserter(captured_sources_));
  }

  if (type == DesktopMediaList::Type::kScreen) {
    std::vector<DesktopCapturer::Source> screen_sources;
    screen_sources.reserve(list->GetSourceCount());
    for (int i = 0; i < list->GetSourceCount(); i++) {
      screen_sources.emplace_back(list->GetSource(i), std::string());
    }
#if BUILDFLAG(IS_WIN)
    // Gather the same unique screen IDs used by the electron.screen API in
    // order to provide an association between it and
    // desktopCapturer/getUserMedia. This is only required when using the
    // DirectX capturer, otherwise the IDs across the APIs already match.
    if (using_directx_capturer_) {
      std::vector<std::string> device_names;
      // Crucially, this list of device names will be in the same order as
      // |screen_sources|.
      if (!webrtc::DxgiDuplicatorController::Instance()->GetDeviceNames(
              &device_names)) {
        HandleFailure();
        return;
      }
      DCHECK_EQ(device_names.size(), screen_sources.size());

      std::vector<HMONITOR> monitors;
      EnumDisplayMonitors(nullptr, nullptr, EnumDisplayMonitorsCallback,
                          reinterpret_cast<LPARAM>(&monitors));

      base::flat_map<std::string, int64_t> device_name_to_id;
      device_name_to_id.reserve(monitors.size());
      for (auto* monitor : monitors) {
        MONITORINFOEX monitorInfo{{sizeof(MONITORINFOEX)}};
        if (!GetMonitorInfo(monitor, &monitorInfo)) {
          continue;
        }

        device_name_to_id[base::WideToUTF8(monitorInfo.szDevice)] =
            display::win::internal::DisplayInfo::DisplayIdFromMonitorInfo(
                monitorInfo);
      }

      int device_name_index = 0;
      for (auto& source : screen_sources) {
        const auto& device_name = device_names[device_name_index++];
        if (auto id_iter = device_name_to_id.find(device_name);
            id_iter != device_name_to_id.end()) {
          source.display_id = base::NumberToString(id_iter->second);
        }
      }
    }
#elif BUILDFLAG(IS_MAC)
    // On Mac, the IDs across the APIs match.
    for (auto& source : screen_sources) {
      source.display_id = base::NumberToString(source.media_list_source.id.id);
    }
#elif BUILDFLAG(SUPPORTS_OZONE_X11)
    // On Linux, with X11, the source id is the numeric value of the
    // display name atom and the display id is either the EDID or the
    // loop index when that display was found (see
    // BuildDisplaysFromXRandRInfo in ui/base/x/x11_display_util.cc)
    const auto monitor_atom_to_display_id = MonitorAtomIdToDisplayId();
    for (auto& source : screen_sources) {
      auto display_id_iter =
          monitor_atom_to_display_id.find(source.media_list_source.id.id);
      if (display_id_iter != monitor_atom_to_display_id.end())
        source.display_id = base::NumberToString(display_id_iter->second);
    }
#endif
    std::move(screen_sources.begin(), screen_sources.end(),
              std::back_inserter(captured_sources_));
  }
}

void DesktopCapturer::HandleSuccess() {
  if (finished_)
    return;
  finished_ = true;
  deadline_.Stop();

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  gin_helper::CallMethod(this, "_onfinished", captured_sources_);

  window_observer_.reset();
  screen_observer_.reset();
  screen_capturer_.reset();
  window_capturer_.reset();

  Unpin();
}

void DesktopCapturer::HandleFailure() {
  if (finished_)
    return;
  finished_ = true;
  deadline_.Stop();

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);

  gin_helper::CallMethod(this, "_onerror", "Failed to get sources.");

  window_observer_.reset();
  screen_observer_.reset();
  screen_capturer_.reset();
  window_capturer_.reset();

  Unpin();
}

// static
gin_helper::Handle<DesktopCapturer> DesktopCapturer::Create(
    v8::Isolate* isolate) {
  auto handle = gin_helper::CreateHandle(isolate, new DesktopCapturer());

  // Keep reference alive until capturing has finished.
  handle->Pin(isolate);

  return handle;
}

// static
#if !BUILDFLAG(IS_MAC)
bool DesktopCapturer::IsDisplayMediaSystemPickerAvailable() {
  return false;
}
#endif

gin::ObjectTemplateBuilder DesktopCapturer::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::DeprecatedWrappable<
             DesktopCapturer>::GetObjectTemplateBuilder(isolate)
      .SetMethod("startHandling", &DesktopCapturer::StartHandling);
}

const char* DesktopCapturer::GetTypeName() {
  return "DesktopCapturer";
}

}  // namespace electron::api

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("createDesktopCapturer",
                 &electron::api::DesktopCapturer::Create);
  dict.SetMethod(
      "isDisplayMediaSystemPickerAvailable",
      &electron::api::DesktopCapturer::IsDisplayMediaSystemPickerAvailable);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_desktop_capturer, Initialize)
