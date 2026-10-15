// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license.
#include "shell/browser/native_video_frame_registry.h"

#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/timer/timer.h"
#include "base/uuid.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "ui/gfx/color_space.h"

namespace electron {
namespace {
bool IsNativeHandle(const gfx::GpuMemoryBufferHandle& handle) {
#if BUILDFLAG(IS_LINUX)
  if (handle.type != gfx::NATIVE_PIXMAP)
    return false;
  const auto& planes = handle.native_pixmap_handle().planes;
  if (planes.empty() || planes.size() > 4)
    return false;
  for (const auto& plane : planes) {
    if (!plane.fd.is_valid() || plane.stride == 0 || plane.size == 0)
      return false;
  }
  return true;
#elif BUILDFLAG(IS_MAC)
  return handle.type == gfx::IO_SURFACE_BUFFER && handle.io_surface();
#elif BUILDFLAG(IS_WIN)
  return handle.type == gfx::DXGI_SHARED_HANDLE &&
         handle.dxgi_handle().buffer_handle();
#else
  return false;
#endif
}
}  // namespace

struct NativeVideoFrameRegistry::Entry {
  gfx::GpuMemoryBufferHandle handle;
  mojom::NativeVideoFrameInfoPtr info;
  mojo::Remote<mojom::NativeVideoFrameLease> lease;
  base::OneShotTimer expiry;

  void Release() {
    expiry.Stop();
    handle = gfx::GpuMemoryBufferHandle();
    lease.reset();
  }
};

DOCUMENT_USER_DATA_KEY_IMPL(NativeVideoFrameRegistry);

NativeVideoFrameRegistry::NativeVideoFrameRegistry(
    content::RenderFrameHost* frame)
    : DocumentUserData(frame) {}
NativeVideoFrameRegistry::~NativeVideoFrameRegistry() = default;

void NativeVideoFrameRegistry::Bind(
    content::RenderFrameHost* frame,
    mojo::PendingAssociatedReceiver<mojom::NativeVideoFrameHost> receiver) {
  GetOrCreateForCurrentDocument(frame)->receivers_.Add(
      GetForCurrentDocument(frame), std::move(receiver));
}

void NativeVideoFrameRegistry::Export(
    gfx::GpuMemoryBufferHandle handle,
    mojom::NativeVideoFrameInfoPtr info,
    mojo::PendingRemote<mojom::NativeVideoFrameLease> lease,
    ExportCallback callback) {
  if (entries_.size() >= 4 || !lease.is_valid() || !IsNativeHandle(handle) ||
      info->coded_size.IsEmpty() || info->coded_size.width() > 16384 ||
      info->coded_size.height() > 16384 || info->visible_rect.IsEmpty() ||
      !gfx::Rect(info->coded_size).Contains(info->visible_rect)) {
    std::move(callback).Run(
        "", "Invalid native frame or too many pending exports.");
    return;
  }
  const std::string id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  auto entry = std::make_shared<Entry>();
  entry->handle = std::move(handle);
  entry->info = std::move(info);
  entry->lease.Bind(std::move(lease));
  entry->lease.set_disconnect_handler(base::BindOnce(
      &NativeVideoFrameRegistry::Expire, weak_factory_.GetWeakPtr(), id));
  entry->expiry.Start(FROM_HERE, base::Seconds(15),
                      base::BindOnce(&NativeVideoFrameRegistry::Expire,
                                     weak_factory_.GetWeakPtr(), id));
  entries_.emplace(id, std::move(entry));
  std::move(callback).Run(id, "");
}

void NativeVideoFrameRegistry::Expire(const std::string& id) {
  entries_.erase(id);
}

void NativeVideoFrameRegistry::Release(const std::string& id,
                                       ReleaseCallback callback) {
  entries_.erase(id);
  std::move(callback).Run();
}

v8::Local<v8::Value> NativeVideoFrameRegistry::Resolve(
    v8::Isolate* isolate,
    content::RenderFrameHost* frame,
    const std::string& id) {
  auto* registry = frame ? GetForCurrentDocument(frame) : nullptr;
  if (!registry || !registry->entries_.contains(id)) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Unknown, expired or already resolved native VideoFrame handle.");
    return v8::Undefined(isolate);
  }
  auto entry = registry->entries_.extract(id).mapped();
  entry->expiry.Stop();
  entry->lease.set_disconnect_handler(base::OnceClosure());
  gin_helper::Dictionary texture(isolate, v8::Object::New(isolate));
  texture.Set("pixelFormat", entry->info->rgba ? "rgba" : "bgra");
  texture.Set("codedSize", entry->info->coded_size);
  texture.Set("visibleRect", entry->info->visible_rect);
  texture.Set("contentRect", entry->info->visible_rect);
  texture.Set("timestamp", entry->info->timestamp_us);
  texture.Set("colorSpace", gfx::ColorSpace::CreateSRGB());
  gin_helper::Dictionary handle(isolate, v8::Object::New(isolate));
#if BUILDFLAG(IS_LINUX)
  const auto& pixmap = entry->handle.native_pixmap_handle();
  gin_helper::Dictionary native(isolate, v8::Object::New(isolate));
  auto planes = base::ToVector(pixmap.planes, [isolate](const auto& plane) {
    gin_helper::Dictionary value(isolate, v8::Object::New(isolate));
    value.Set("stride", plane.stride);
    value.Set("offset", plane.offset);
    value.Set("size", plane.size);
    value.Set("fd", plane.fd.get());
    return value;
  });
  native.Set("planes", planes);
  native.Set("modifier", base::NumberToString(pixmap.modifier));
  native.Set("supportsZeroCopyWebGpuImport",
             pixmap.supports_zero_copy_webgpu_import);
  handle.Set("nativePixmap", native);
#elif BUILDFLAG(IS_MAC)
  uintptr_t pointer =
      reinterpret_cast<uintptr_t>(entry->handle.io_surface().get());
  handle.Set("ioSurface",
             Buffer::Copy(isolate, base::byte_span_from_ref(pointer))
                 .ToLocalChecked());
#elif BUILDFLAG(IS_WIN)
  uintptr_t pointer =
      reinterpret_cast<uintptr_t>(entry->handle.dxgi_handle().buffer_handle());
  handle.Set("ntHandle",
             Buffer::Copy(isolate, base::byte_span_from_ref(pointer))
                 .ToLocalChecked());
#endif
  texture.Set("handle", handle);
  gin_helper::Dictionary result(isolate, v8::Object::New(isolate));
  result.Set("textureInfo", texture);
  // The callback owns the native allocation and source lease even after
  // navigation. A native consumer must acknowledge completion before release.
  result.SetMethod(
      "release",
      base::BindRepeating(
          [](std::shared_ptr<Entry> entry, v8::Isolate* isolate) {
            gin_helper::Promise<void> promise(isolate);
            auto handle = promise.GetHandle();
            if (!entry->lease.is_bound()) {
              promise.Resolve();
              return handle;
            }
            entry->lease->Release(
                mojo::WrapCallbackWithDefaultInvokeIfNotRun(base::BindOnce(
                    [](std::shared_ptr<Entry> entry,
                       gin_helper::Promise<void> promise) {
                      entry->Release();
                      v8::HandleScope scope(promise.isolate());
                      if (!promise.GetContext().IsEmpty())
                        promise.Resolve();
                    },
                    entry, std::move(promise))));
            return handle;
          },
          entry));
  return result.GetHandle();
}
}  // namespace electron
