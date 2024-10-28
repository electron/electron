
// Copyright (c) 2024 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "shell/common/gin_converters/osr_converter.h"

#include "gin/dictionary.h"
#include "v8-external.h"
#include "v8-function.h"

#include <string>

#include "base/containers/to_vector.h"
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
    default:
      NOTREACHED_NORETURN();
  }
}

std::string OsrWidgetTypeToString(content::WidgetType type) {
  switch (type) {
    case content::WidgetType::kPopup:
      return "popup";
    case content::WidgetType::kFrame:
      return "frame";
    default:
      NOTREACHED_NORETURN();
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

  bool IsTextureReleased() const { return holder_ == nullptr; }

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

  auto releaserHolder = v8::External::New(isolate, monitor);
  auto releaserFunc = [](const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto* holder = static_cast<OffscreenReleaseHolderMonitor*>(
        info.Data().As<v8::External>()->Value());
    // Release the shared texture, so that future frames can be generated.
    holder->ReleaseTexture();
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
  dict.Set("widgetType", OsrWidgetTypeToString(val.widget_type));

  gin::Dictionary metadata(isolate, v8::Object::New(isolate));
  metadata.Set("captureUpdateRect", val.capture_update_rect);
  metadata.Set("regionCaptureRect", val.region_capture_rect);
  metadata.Set("sourceSize", val.source_size);
  metadata.Set("frameCount", val.frame_count);
  dict.Set("metadata", ConvertToV8(isolate, metadata));

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  auto handle_buf = node::Buffer::Copy(
      isolate,
      reinterpret_cast<char*>(
          const_cast<uintptr_t*>(&val.shared_texture_handle)),
      sizeof(val.shared_texture_handle));
  dict.Set("sharedTextureHandle", handle_buf.ToLocalChecked());
#elif BUILDFLAG(IS_LINUX)
  auto v8_planes = base::ToVector(val.planes, [isolate](const auto& plane) {
    gin::Dictionary v8_plane(isolate, v8::Object::New(isolate));
    v8_plane.Set("stride", plane.stride);
    v8_plane.Set("offset", plane.offset);
    v8_plane.Set("size", plane.size);
    v8_plane.Set("fd", plane.fd);
    return v8_plane;
  });
  dict.Set("planes", v8_planes);
  dict.Set("modifier", base::NumberToString(val.modifier));
#endif

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
            auto* iso = data.GetIsolate();
            // Emit warning only once
            static std::once_flag flag;
            std::call_once(flag, [=] {
              electron::util::EmitWarning(
                  iso,
                  "[OSR TEXTURE LEAKED] When using OSR with "
                  "`useSharedTexture`, `texture.release()` "
                  "must be called explicitly as soon as the texture is "
                  "copied to your rendering system. "
                  "Otherwise, it will soon drain the underlying "
                  "framebuffer and prevent future frames from being generated.",
                  "SharedTextureOSRNotReleased");
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
