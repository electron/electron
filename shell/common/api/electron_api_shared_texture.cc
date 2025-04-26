// Copyright (c) 2025 Reito <reito@chromium.org>
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_shared_texture.h"

#include "base/base64.h"
#include "base/command_line.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions_internal.h"
#include "components/viz/common/resources/shared_image_format_utils.h"
#include "content/browser/compositor/image_transport_factory.h"  // nogncheck
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/service/shared_image/shared_image_factory.h"
#include "gpu/ipc/client/client_shared_image_interface.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/common/exported_shared_image.mojom-shared.h"
#include "gpu/ipc/common/exported_shared_image_mojom_traits.h"
#include "media/base/format_utils.h"
#include "media/base/video_frame.h"
#include "media/mojo/mojom/video_frame_mojom_traits.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "third_party/blink/renderer/modules/webcodecs/video_frame.h"  // nogncheck
#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"  // nogncheck
#include "ui/compositor/compositor.h"

#if BUILDFLAG(IS_LINUX)
#include "base/posix/eintr_wrapper.h"
#endif

namespace {

bool IsBrowserProcess() {
  static int is_browser_process = -1;
  if (is_browser_process == -1) {
    // Browser process does not specify a type.
    is_browser_process = base::CommandLine::ForCurrentProcess()
                             ->GetSwitchValueASCII("type")
                             .empty();
  }

  return is_browser_process == 1;
}

gpu::ContextSupport* GetContextSupport() {
  if (IsBrowserProcess()) {
    auto* factory = content::ImageTransportFactory::GetInstance();
    return factory->GetContextFactory()
        ->SharedMainThreadRasterContextProvider()
        ->ContextSupport();
  } else {
    return blink::SharedGpuContext::ContextProviderWrapper()
        ->ContextProvider()
        .ContextSupport();
  }
}

gpu::SharedImageInterface* GetSharedImageInterface() {
  if (IsBrowserProcess()) {
    auto* factory = content::ImageTransportFactory::GetInstance();
    return factory->GetContextFactory()
        ->SharedMainThreadRasterContextProvider()
        ->SharedImageInterface();
  } else {
    return blink::SharedGpuContext::SharedImageInterfaceProvider()
        ->SharedImageInterface();
  }
}

std::string TransferVideoPixelFormatToString(media::VideoPixelFormat format) {
  switch (format) {
    case media::PIXEL_FORMAT_ARGB:
      return "bgra";
    case media::PIXEL_FORMAT_ABGR:
      return "rgba";
    default:
      NOTREACHED();
  }
}

struct ImportedSharedTexture {
  // Metadata
  gfx::Size coded_size;
  gfx::Rect visible_rect;
  int64_t timestamp;
  media::VideoPixelFormat pixel_format;

  // Holds a reference to prevent it from being destroyed.
  scoped_refptr<gpu::ClientSharedImage> client_shared_image;
  gpu::SyncToken frame_creation_sync_token;
  gpu::SyncToken release_sync_token;
  base::OnceClosure release_callback;

  // Create VideoFrame from the shared image.
  v8::Local<v8::Value> CreateVideoFrame(v8::Isolate* isolate);

  // Monitor garbage collection.
  std::unique_ptr<v8::Persistent<v8::Value>> persistent_;
  void ResetPersistent() const { persistent_->Reset(); }
  v8::Persistent<v8::Value>* CreatePersistent(v8::Isolate* isolate,
                                              v8::Local<v8::Value> value) {
    persistent_ = std::make_unique<v8::Persistent<v8::Value>>(isolate, value);
    return persistent_.get();
  }

  // Release the shared image.
  bool IsReleased() const { return client_shared_image.get() == nullptr; }
  void Release();

  // Transfer to other Chromium processes.
  v8::Local<v8::Value> StartTransferSharedTexture(v8::Isolate* isolate);
};

void OnVideoFrameMailboxReleased(ImportedSharedTexture* ist,
                                 const gpu::SyncToken& sync_token) {
  ist->release_sync_token = sync_token;
}

void ImportedSharedTexture::Release() {
  if (IsReleased()) {
    LOG(ERROR) << "Imported shared texture already released.";
    return;
  }

  auto* sii = GetSharedImageInterface();
  if (!this->release_sync_token.HasData()) {
    this->release_sync_token = sii->GenVerifiedSyncToken();
  }

  this->client_shared_image->UpdateDestructionSyncToken(
      this->release_sync_token);

  if (this->release_callback) {
    GetContextSupport()->SignalSyncToken(this->release_sync_token,
                                         std::move(this->release_callback));
  }

  client_shared_image.reset();
}

v8::Local<v8::Value> ImportedSharedTexture::CreateVideoFrame(
    v8::Isolate* isolate) {
  auto* current_script_state = blink::ScriptState::ForCurrentRealm(isolate);
  auto* current_execution_context =
      blink::ToExecutionContext(current_script_state);

  auto si = this->client_shared_image;

  auto cb = base::BindOnce(OnVideoFrameMailboxReleased, this);

  scoped_refptr<media::VideoFrame> raw_frame =
      media::VideoFrame::WrapSharedImage(
          this->pixel_format, si, this->frame_creation_sync_token,
          std::move(cb), this->coded_size, this->visible_rect, this->coded_size,
          base::Microseconds(this->timestamp));

  blink::VideoFrame* frame = blink::MakeGarbageCollected<blink::VideoFrame>(
      raw_frame, current_execution_context);
  return blink::ToV8Traits<blink::VideoFrame>::ToV8(current_script_state,
                                                    frame);
}

v8::Local<v8::Value> ImportedSharedTexture::StartTransferSharedTexture(
    v8::Isolate* isolate) {
  auto exported = this->client_shared_image->Export();

  // Use mojo to serialize the exported shared image.
  mojo::Message message(0, 0, MOJO_CREATE_MESSAGE_FLAG_UNLIMITED_SIZE, 0);
  mojo::internal::MessageFragment<
      gpu::mojom::internal::ExportedSharedImage_Data>
      data(message);
  data.Allocate();
  mojo::internal::Serializer<gpu::mojom::ExportedSharedImageDataView,
                             gpu::ExportedSharedImage>::Serialize(exported,
                                                                  data);

  auto encoded = base::Base64Encode(UNSAFE_BUFFERS(
      base::span(message.payload(), message.payload_num_bytes())));
  gin::Dictionary root(isolate, v8::Object::New(isolate));
  root.Set("transfer", encoded);

  root.Set("pixelFormat", TransferVideoPixelFormatToString(this->pixel_format));
  root.Set("codedSize", this->coded_size);
  root.Set("visibleRect", this->visible_rect);
  root.Set("timestamp", this->timestamp);

  return gin::ConvertToV8(isolate, root);
}

void PersistentCallbackPass2(
    const v8::WeakCallbackInfo<ImportedSharedTexture>& data) {
  // Emit warning only once
  static std::once_flag flag;
  std::call_once(flag, [] {
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce([] {
          electron::util::EmitWarning(
              "SharedTextureImported was garbage collected before calling "
              "`release()`. You have to manually release the resource once "
              "you're done with it.",
              "SharedTextureForgetToRelease");
        }));
  });
}

void PersistentCallbackPass1(
    const v8::WeakCallbackInfo<ImportedSharedTexture>& data) {
  auto* imp = data.GetParameter();
  if (!imp->IsReleased()) {
    // Release it for user here.
    imp->Release();
    // Emit a warning when the user didn't properly manually release the
    // texture, output it in the second pass callback.
    data.SetSecondPassCallback(PersistentCallbackPass2);
  }
  // We are responsible for resetting the persistent handle.
  imp->ResetPersistent();
  // Finally, release the import monitor;
  delete imp;
}

void ImportedTextureGetVideoFrame(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* isolate = info.GetIsolate();
  auto* tex = static_cast<ImportedSharedTexture*>(
      info.Data().As<v8::External>()->Value());

  if (tex->IsReleased()) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "The shared texture has been released.");
    return;
  }

  auto ret = tex->CreateVideoFrame(isolate);
  info.GetReturnValue().Set(ret);
}

void ImportedTextureStartTransferSharedTexture(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* isolate = info.GetIsolate();
  auto* tex = static_cast<ImportedSharedTexture*>(
      info.Data().As<v8::External>()->Value());

  if (tex->IsReleased()) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "The shared texture has been released.");
    return;
  }

  auto ret = tex->StartTransferSharedTexture(isolate);
  info.GetReturnValue().Set(ret);
}

void ImportedTextureRelease(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* tex = static_cast<ImportedSharedTexture*>(
      info.Data().As<v8::External>()->Value());

  auto cb = info[0];
  if (cb->IsFunction()) {
    auto* isolate = info.GetIsolate();
    gin::ConvertFromV8(isolate, cb, &tex->release_callback);
  }

  // Release the shared texture, so that future frames can be generated.
  tex->Release();

  // Release the wrapper happens at GC persistent callback, don't release here.
}

v8::Local<v8::Value> CreateImportedSharedTextureFromSharedImage(
    v8::Isolate* isolate,
    ImportedSharedTexture* imported) {
  auto imported_wrapped = v8::External::New(isolate, imported);
  gin::Dictionary root(isolate, v8::Object::New(isolate));

  auto releaser = v8::Function::New(isolate->GetCurrentContext(),
                                    ImportedTextureRelease, imported_wrapped)
                      .ToLocalChecked();

  auto get_video_frame =
      v8::Function::New(isolate->GetCurrentContext(),
                        ImportedTextureGetVideoFrame, imported_wrapped)
          .ToLocalChecked();

  auto start_transfer =
      v8::Function::New(isolate->GetCurrentContext(),
                        ImportedTextureStartTransferSharedTexture,
                        imported_wrapped)
          .ToLocalChecked();

  root.Set("release", releaser);
  root.Set("getVideoFrame", get_video_frame);
  root.Set("startTransferSharedTexture", start_transfer);

  auto root_local = gin::ConvertToV8(isolate, root);
  auto* persistent = imported->CreatePersistent(isolate, root_local);

  persistent->SetWeak(imported, PersistentCallbackPass1,
                      v8::WeakCallbackType::kParameter);

  return root_local;
}

struct ImportSharedTextureInfoPlane {
  // The strides and offsets in bytes to be used when accessing the buffers
  // via a memory mapping. One per plane per entry. Size in bytes of the
  // plane is necessary to map the buffers.
  uint32_t stride;
  uint64_t offset;
  uint64_t size;

  // File descriptor for the underlying memory object (usually dmabuf).
  int fd = 0;
};

struct ImportSharedTextureInfo {
  // The pixel format of the shared texture, RGBA or BGRA depends on platform.
  media::VideoPixelFormat pixel_format;

  // The full dimensions of the video frame data.
  gfx::Size coded_size;

  // A subsection of [0, 0, coded_size().width(), coded_size.height()].
  // In OSR case, it is expected to have the full area of the section.
  gfx::Rect visible_rect;

  // The color space of the video frame.
  gfx::ColorSpace color_space = gfx::ColorSpace::CreateSRGB();

  // The capture timestamp, microseconds since capture start
  int64_t timestamp = 0;

#if BUILDFLAG(IS_WIN)
  // On Windows, it must be a NT HANDLE (CreateSharedHandle) to the shared
  // texture, it can't be a deprecated non-NT HANDLE (GetSharedHandle). This
  // must be a handle already duplicated for the current process and can be
  // owned by this.
  uintptr_t nt_handle = 0;
#elif BUILDFLAG(IS_APPLE)
  // On macOS, it is an IOSurfaceRef, this must be a valid IOSurface at the
  // current process.
  uintptr_t io_surface = 0;
#elif BUILDFLAG(IS_LINUX)
  // On Linux, to be implemented.
  std::vector<ImportSharedTextureInfoPlane> planes;
  uint64_t modifier = gfx::NativePixmapHandle::kNoModifier;
  bool supports_zero_copy_webgpu_import = false;
#endif
};

}  // namespace

namespace gin {

template <>
struct Converter<ImportSharedTextureInfo> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ImportSharedTextureInfo* out) {
    if (!val->IsObject())
      return false;
    gin::Dictionary dict(isolate, val.As<v8::Object>());

    std::string pixel_format_str;
    if (dict.Get("pixelFormat", &pixel_format_str)) {
      if (pixel_format_str == "bgra")
        out->pixel_format = media::PIXEL_FORMAT_ARGB;
      else if (pixel_format_str == "rgba")
        out->pixel_format = media::PIXEL_FORMAT_ABGR;
      else
        return false;
    }

    dict.Get("codedSize", &out->coded_size);
    if (!dict.Get("visibleRect", &out->visible_rect)) {
      out->visible_rect = gfx::Rect(out->coded_size);
    }
    dict.Get("colorSpace", &out->color_space);
    dict.Get("timestamp", &out->timestamp);

    gin::Dictionary shared_texture(isolate, val.As<v8::Object>());
    if (!dict.Get("sharedTextureData", &shared_texture)) {
      return false;
    }

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_APPLE)
    auto GetNativeHandle = [&](const std::string& property_key,
                               uintptr_t* output) {
      v8::Local<v8::Value> handle_buf;
      if (shared_texture.Get(property_key, &handle_buf) &&
          node::Buffer::HasInstance(handle_buf)) {
        char* data = node::Buffer::Data(handle_buf);
        if (node::Buffer::Length(handle_buf) == sizeof(uintptr_t)) {
          *output = *reinterpret_cast<uintptr_t*>(data);
        }
      }
    };
#endif

#if BUILDFLAG(IS_WIN)
    GetNativeHandle("ntHandle", &out->nt_handle);
#elif BUILDFLAG(IS_APPLE)
    GetNativeHandle("ioSurface", &out->io_surface);
#elif BUILDFLAG(IS_LINUX)
    v8::Local<v8::Object> native_pixmap;
    if (shared_texture.Get("nativePixmap", &native_pixmap)) {
      gin::Dictionary v8_native_pixmap(isolate, native_pixmap);
      v8::Local<v8::Array> v8_planes;
      if (v8_native_pixmap.Get("planes", &v8_planes)) {
        out->planes.clear();
        for (uint32_t i = 0; i < v8_planes->Length(); ++i) {
          v8::Local<v8::Value> v8_item =
              v8_planes->Get(isolate->GetCurrentContext(), i).ToLocalChecked();
          gin::Dictionary v8_plane(isolate, v8_item.As<v8::Object>());
          ImportSharedTextureInfoPlane plane;
          v8_plane.Get("stride", &plane.stride);
          v8_plane.Get("offset", &plane.offset);
          v8_plane.Get("size", &plane.size);
          v8_plane.Get("fd", &plane.fd);
          out->planes.push_back(plane);
        }
      }
      std::string modifier_str;
      if (v8_native_pixmap.Get("modifier", &modifier_str)) {
        base::StringToUint64(modifier_str, &out->modifier);
      }
      v8_native_pixmap.Get("supportsZeroCopyWebGpuImport",
                           &out->supports_zero_copy_webgpu_import);
    }
#endif

    return true;
  }
};

}  // namespace gin

namespace electron::api::shared_texture {

v8::Local<v8::Value> ImportSharedTexture(v8::Isolate* isolate,
                                         v8::Local<v8::Value> options) {
  ImportSharedTextureInfo shared_texture{};
  if (!gin::ConvertFromV8(isolate, options, &shared_texture)) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "Invalid shared texture info object");
    return v8::Null(isolate);
  }

  gfx::GpuMemoryBufferHandle gmb_handle;
#if BUILDFLAG(IS_WIN)
  if (shared_texture.nt_handle == 0) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError("Invalid ntHandle value");
    return v8::Null(isolate);
  }

  auto handle = reinterpret_cast<HANDLE>(shared_texture.nt_handle);

  HANDLE dup_handle;
  // Duplicate the handle to allow scoped handle close it.
  if (!DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(),
                       &dup_handle, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "Unable to duplicate handle.");
    return v8::Null(isolate);
  }

  auto dxgi_handle = gfx::DXGIHandle(base::win::ScopedHandle(dup_handle));
  gmb_handle = gfx::GpuMemoryBufferHandle(std::move(dxgi_handle));
#elif BUILDFLAG(IS_APPLE)
  if (shared_texture.io_surface == 0) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError("Invalid ioSurface value");
    return v8::Null(isolate);
  }

  gmb_handle.type = gfx::IO_SURFACE_BUFFER;
  auto io_surface = reinterpret_cast<IOSurfaceRef>(shared_texture.io_surface);

  // Retain the io_surface reference to increase the reference count.
  gmb_handle.io_surface = base::apple::ScopedCFTypeRef<IOSurfaceRef>(
      io_surface, base::scoped_policy::RETAIN);
#elif BUILDFLAG(IS_LINUX)
  gfx::NativePixmapHandle pixmap;
  pixmap.modifier = shared_texture.modifier;
  pixmap.supports_zero_copy_webgpu_import =
      shared_texture.supports_zero_copy_webgpu_import;

  for (const auto& plane : shared_texture.planes) {
    gfx::NativePixmapPlane plane_info;
    plane_info.stride = plane.stride;
    plane_info.offset = plane.offset;
    plane_info.size = plane.size;

    // Duplicate fd, otherwise the process may already have ownership.
    int checked_dup = HANDLE_EINTR(dup(plane.fd));
    plane_info.fd = base::ScopedFD(checked_dup);

    pixmap.planes.push_back(std::move(plane_info));
  }

  gmb_handle = gfx::GpuMemoryBufferHandle(std::move(pixmap));
#endif

  gfx::Size coded_size = shared_texture.coded_size;
  media::VideoPixelFormat pixel_format = shared_texture.pixel_format;
  gfx::ColorSpace color_space = shared_texture.color_space;

  auto buffer_format = media::VideoPixelFormatToGfxBufferFormat(pixel_format);
  if (!buffer_format.has_value()) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "Invalid shared texture buffer format");
    return v8::Null(isolate);
  }

  auto* sii = GetSharedImageInterface();
  gpu::SharedImageUsageSet shared_image_usage =
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_APPLE)
      gpu::SHARED_IMAGE_USAGE_SCANOUT | gpu::SHARED_IMAGE_USAGE_GLES2_READ |
      gpu::SHARED_IMAGE_USAGE_RASTER_READ |
      gpu::SHARED_IMAGE_USAGE_DISPLAY_READ |
      gpu::SHARED_IMAGE_USAGE_WEBGPU_READ;
#else
      gpu::SHARED_IMAGE_USAGE_SCANOUT | gpu::SHARED_IMAGE_USAGE_GLES2_READ |
      gpu::SHARED_IMAGE_USAGE_RASTER_READ |
      gpu::SHARED_IMAGE_USAGE_DISPLAY_READ;
#endif

  auto si_format = viz::GetSharedImageFormat(buffer_format.value());
  auto si =
      sii->CreateSharedImage({si_format, coded_size, color_space,
                              shared_image_usage, "SharedTextureVideoFrame"},
                             std::move(gmb_handle));

  ImportedSharedTexture* imported = new ImportedSharedTexture();
  imported->pixel_format = shared_texture.pixel_format;
  imported->coded_size = shared_texture.coded_size;
  imported->visible_rect = shared_texture.visible_rect;
  imported->timestamp = shared_texture.timestamp;
  imported->frame_creation_sync_token = si->creation_sync_token();
  imported->client_shared_image = std::move(si);

  return CreateImportedSharedTextureFromSharedImage(isolate, imported);
}

v8::Local<v8::Value> FinishTransferSharedTexture(v8::Isolate* isolate,
                                                 v8::Local<v8::Value> options) {
  ImportSharedTextureInfo partial{};
  gin::ConvertFromV8(isolate, options, &partial);

  std::string transfer;
  gin::Dictionary dict(isolate, options.As<v8::Object>());
  dict.Get("transfer", &transfer);
  auto transfer_data = base::Base64Decode(transfer);

  // Use mojo to deserialize the exported shared image.
  mojo::Message message(transfer_data.value(), {});
  mojo::internal::MessageFragment<
      gpu::mojom::internal::ExportedSharedImage_Data>
      data(message);
  data.Claim(message.mutable_payload());

  gpu::ExportedSharedImage exported;
  mojo::internal::Serializer<gpu::mojom::ExportedSharedImageDataView,
                             gpu::ExportedSharedImage>::Deserialize(data.data(),
                                                                    &exported,
                                                                    &message);

  auto* sii = GetSharedImageInterface();
  auto si = sii->ImportSharedImage(std::move(exported));

  ImportedSharedTexture* imported = new ImportedSharedTexture();
  imported->pixel_format = partial.pixel_format;
  imported->coded_size = partial.coded_size;
  imported->visible_rect = partial.visible_rect;
  imported->timestamp = partial.timestamp;
  imported->frame_creation_sync_token = sii->GenVerifiedSyncToken();
  imported->client_shared_image = std::move(si);

  return CreateImportedSharedTextureFromSharedImage(isolate, imported);
}

}  // namespace electron::api::shared_texture

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("importSharedTexture",
                 &electron::api::shared_texture::ImportSharedTexture);
  dict.SetMethod("finishTransferSharedTexture",
                 &electron::api::shared_texture::FinishTransferSharedTexture);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_shared_texture, Initialize)
