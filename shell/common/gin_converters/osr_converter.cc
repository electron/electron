// Copyright (c) 2025 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/osr_converter.h"

#include "gin/dictionary.h"
#include "v8-external.h"
#include "v8-function.h"

#include <string>

#include "base/containers/to_vector.h"
#include "base/task/single_thread_task_runner.h"
#if BUILDFLAG(IS_LINUX)
#include "base/strings/string_number_conversions.h"
#endif
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/optional_converter.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"

namespace gin {

namespace {
std::string OsrVideoPixelFormatToString(media::VideoPixelFormat format) {
  switch (format) {
    case media::PIXEL_FORMAT_ARGB:
      return "bgra";
    case media::PIXEL_FORMAT_ABGR:
      return "rgba";
    case media::PIXEL_FORMAT_RGBAF16:
      return "rgbaf16";
    default:
      NOTREACHED();
  }
}

std::string OsrWidgetTypeToString(content::WidgetType type) {
  switch (type) {
    case content::WidgetType::kPopup:
      return "popup";
    case content::WidgetType::kFrame:
      return "frame";
    default:
      NOTREACHED();
  }
}

struct OffscreenReleaseHolderMonitor {
  explicit OffscreenReleaseHolderMonitor(
      electron::OffscreenReleaserHolder* holder)
      : holder_(holder) {
    CHECK(holder);
  }

  void ReleaseTexture() {
    delete holder_;
    holder_ = nullptr;
  }

  [[nodiscard]] bool IsTextureReleased() const { return holder_ == nullptr; }

  v8::Persistent<v8::Value>* CreatePersistent(v8::Isolate* isolate,
                                              v8::Local<v8::Value> value) {
    persistent_ = std::make_unique<v8::Persistent<v8::Value>>(isolate, value);
    return persistent_.get();
  }

  void ResetPersistent() const { persistent_->Reset(); }

 private:
  raw_ptr<electron::OffscreenReleaserHolder> holder_;
  std::unique_ptr<v8::Persistent<v8::Value>> persistent_;
};

}  // namespace

// static
v8::Local<v8::Value> Converter<electron::OffscreenSharedTextureValue>::ToV8(
    v8::Isolate* isolate,
    const electron::OffscreenSharedTextureValue& val) {
  gin::Dictionary root(isolate, v8::Object::New(isolate));

  // Create a monitor to hold the releaser holder, which enables us to
  // monitor whether the user explicitly released the texture before
  // GC collects the object.
  auto* monitor = new OffscreenReleaseHolderMonitor(val.releaser_holder);

  auto releaserHolder =
      v8::External::New(isolate, monitor, v8::kExternalPointerTypeTagDefault);
  auto releaserFunc = [](const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto* mon = static_cast<OffscreenReleaseHolderMonitor*>(
        info.Data().As<v8::External>()->Value(
            v8::kExternalPointerTypeTagDefault));
    // Release the shared texture, so that future frames can be generated.
    mon->ReleaseTexture();
    // Release the monitor happens at GC, don't release here.
  };
  auto releaser = v8::Function::New(isolate->GetCurrentContext(), releaserFunc,
                                    releaserHolder)
                      .ToLocalChecked();

  root.Set("release", releaser);

  gin::Dictionary dict(isolate, v8::Object::New(isolate));
  dict.Set("pixelFormat", OsrVideoPixelFormatToString(val.pixel_format));
  dict.Set("codedSize", val.coded_size);
  dict.Set("visibleRect", val.visible_rect);
  dict.Set("contentRect", val.content_rect);
  dict.Set("timestamp", val.timestamp);
  dict.Set("colorSpace", val.color_space);
  dict.Set("widgetType", OsrWidgetTypeToString(val.widget_type));

  gin::Dictionary metadata(isolate, v8::Object::New(isolate));
  metadata.Set("captureUpdateRect", val.capture_update_rect);
  metadata.Set("regionCaptureRect", val.region_capture_rect);
  metadata.Set("sourceSize", val.source_size);
  metadata.Set("frameCount", val.frame_count);
  dict.Set("metadata", ConvertToV8(isolate, metadata));

  gin::Dictionary sharedTexture(isolate, v8::Object::New(isolate));
#if BUILDFLAG(IS_WIN)
  sharedTexture.Set(
      "ntHandle",
      electron::Buffer::Copy(
          isolate, base::byte_span_from_ref(val.shared_texture_handle))
          .ToLocalChecked());
#elif BUILDFLAG(IS_MAC)
  sharedTexture.Set(
      "ioSurface",
      electron::Buffer::Copy(
          isolate, base::byte_span_from_ref(val.shared_texture_handle))
          .ToLocalChecked());
#elif BUILDFLAG(IS_LINUX)
  gin::Dictionary nativePixmap(isolate, v8::Object::New(isolate));
  auto v8_planes = base::ToVector(val.planes, [isolate](const auto& plane) {
    gin::Dictionary v8_plane(isolate, v8::Object::New(isolate));
    v8_plane.Set("stride", plane.stride);
    v8_plane.Set("offset", plane.offset);
    v8_plane.Set("size", plane.size);
    v8_plane.Set("fd", plane.fd);
    return v8_plane;
  });
  nativePixmap.Set("planes", v8_planes);
  nativePixmap.Set("modifier", base::NumberToString(val.modifier));
  nativePixmap.Set("supportsZeroCopyWebGpuImport",
                   val.supports_zero_copy_webgpu_import);
  sharedTexture.Set("nativePixmap", ConvertToV8(isolate, nativePixmap));
#endif

  dict.Set("handle", ConvertToV8(isolate, sharedTexture));
  root.Set("textureInfo", ConvertToV8(isolate, dict));
  auto root_local = ConvertToV8(isolate, root);

  // Create a persistent reference of the object, so that we can check the
  // monitor again when GC collects this object.
  auto* tex_persistent = monitor->CreatePersistent(isolate, root_local);
  tex_persistent->SetWeak(
      monitor,
      [](const v8::WeakCallbackInfo<OffscreenReleaseHolderMonitor>& data) {
        auto* monitor = data.GetParameter();
        if (!monitor->IsTextureReleased()) {
          // Emit a warning when user didn't properly manually release the
          // texture, output it in second pass callback.
          data.SetSecondPassCallback([](const v8::WeakCallbackInfo<
                                         OffscreenReleaseHolderMonitor>& data) {
            // Emit warning only once
            static std::once_flag flag;
            std::call_once(flag, [=] {
              base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
                  FROM_HERE, base::BindOnce([] {
                    electron::util::EmitWarning(
                        "Offscreen rendering shared texture was garbage "
                        "collected before calling `release()`. When using OSR "
                        "with `useSharedTexture: true`, `texture.release()` "
                        "must be called explicitly as soon as the texture is "
                        "copied to your rendering system. Otherwise, it will "
                        "soon drain the underlying frame pool and prevent "
                        "future frames from being sent.",
                        "OSRSharedTextureNotReleased");
                  }));
            });
          });
        }
        // We are responsible for resetting the persistent handle.
        monitor->ResetPersistent();
        // Finally, release the holder monitor.
        delete monitor;
      },
      v8::WeakCallbackType::kParameter);

  return root_local;
}

}  // namespace gin
