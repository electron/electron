
// Copyright (c) 2024 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "shell/common/gin_converters/osr_converter.h"

#include "gin/dictionary.h"
#include "v8-external.h"
#include "v8-function.h"

#include <string>

#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/node_includes.h"

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

}  // namespace

// static
v8::Local<v8::Value> Converter<electron::OffscreenSharedTextureValue>::ToV8(
    v8::Isolate* isolate,
    const electron::OffscreenSharedTextureValue& val) {
  gin::Dictionary root(isolate, v8::Object::New(isolate));

  auto releaserHolder = v8::External::New(isolate, val.releaser_holder);
  auto releaserFunc = [](const v8::FunctionCallbackInfo<v8::Value>& info) {
    // Delete the holder to release the shared texture.
    delete static_cast<electron::OffscreenReleaserHolder*>(
        info.Data().As<v8::External>()->Value());
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
  dict.Set("frameCount", val.frame_count);
  dict.Set("widgetType", OsrWidgetTypeToString(val.widget_type));
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  auto handle_buf = node::Buffer::Copy(
      isolate,
      reinterpret_cast<char*>(
          const_cast<uintptr_t*>(&val.shared_texture_handle)),
      sizeof(val.shared_texture_handle));
  dict.Set("sharedTextureHandle", handle_buf.ToLocalChecked());
#elif BUILDFLAG(IS_LINUX)
  std::vector<gin::Dictionary> v8_planes;
  for (size_t i = 0; i < val.planes.size(); ++i) {
    auto plane = val.planes[i];
    gin::Dictionary v8_plane(isolate, v8::Object::New(isolate));
    v8_plane.Set("stride", plane.stride);
    v8_plane.Set("offset", plane.offset);
    v8_plane.Set("size", plane.size);
    v8_plane.Set("fd", plane.fd);
    v8_planes.push_back(v8_plane);
  }
  dict.Set("planes", v8_planes);
  dict.Set("modifier", std::to_string(val.modifier));
#endif

  root.Set("textureInfo", ConvertToV8(isolate, dict));
  return ConvertToV8(isolate, root);
}

}  // namespace gin
