// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license.
#include "shell/renderer/api/native_video_frame_export.h"

#include <GLES3/gl3.h>
#include <memory>
#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "content/public/renderer/render_frame.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/client_shared_image.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "media/base/video_frame.h"
#include "media/base/wait_and_replace_sync_token_client.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/native_video_frame.mojom.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_graphics_context_3d_provider.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_video_frame.h"  // nogncheck
#include "third_party/blink/renderer/modules/webcodecs/video_frame.h"  // nogncheck

namespace electron {
namespace {
class SourceLease final : public mojom::NativeVideoFrameLease {
 public:
  explicit SourceLease(scoped_refptr<media::VideoFrame> frame)
      : frame_(std::move(frame)) {}
  void Release(ReleaseCallback callback) override {
    frame_.reset();
    std::move(callback).Run();
  }

 private:
  scoped_refptr<media::VideoFrame> frame_;
};

class NativeVideoFrameExporter
    : public base::RefCounted<NativeVideoFrameExporter> {
 public:
  v8::Local<v8::Promise> Export(v8::Isolate* isolate,
                                v8::Local<v8::Value> value);
  void ReleaseExport(const std::string& id) {
    if (remote_.is_bound())
      remote_->Release(id, base::DoNothing());
  }

 private:
  friend class base::RefCounted<NativeVideoFrameExporter>;
  ~NativeVideoFrameExporter() = default;
  bool Initialize();
  void Complete();
  void Fail(std::string error);
  void Settle(const std::string& id, const std::string& error);
  void Lost();

  std::unique_ptr<blink::WebGraphicsContext3DProvider> provider_;
  std::unique_ptr<gpu::SharedImageTexture> texture_;
  scoped_refptr<media::VideoFrame> source_;
  gfx::GpuMemoryBufferHandle handle_;
  mojom::NativeVideoFrameInfoPtr info_;
  mojo::AssociatedRemote<mojom::NativeVideoFrameHost> remote_;
  std::optional<gin_helper::Promise<std::string>> promise_;
  base::OneShotTimer deadline_;
  unsigned int query_ = 0;
  bool failed_ = false;
  base::WeakPtrFactory<NativeVideoFrameExporter> weak_factory_{this};
};

std::string Validate(const scoped_refptr<media::VideoFrame>& frame) {
  if (!frame)
    return "The VideoFrame is closed.";
  if (!frame->HasSharedImage() || frame->shared_image()->is_software())
    return "A native GPU-backed VideoFrame is required.";
  const auto& image = frame->shared_image();
  const auto type = image->GetGpuMemoryBufferType();
#if BUILDFLAG(IS_LINUX)
  if (type != gfx::NATIVE_PIXMAP)
#elif BUILDFLAG(IS_MAC)
  if (type != gfx::IO_SURFACE_BUFFER)
#elif BUILDFLAG(IS_WIN)
  if (type != gfx::DXGI_SHARED_HANDLE)
#else
  if (true)
#endif
    return "This GPU VideoFrame has no exportable native backing. CPU readback "
           "and GPU copying are disabled.";
  if (image->format() != viz::SinglePlaneFormat::kRGBA_8888 &&
      image->format() != viz::SinglePlaneFormat::kBGRA_8888)
    return "Only native 8-bit RGBA/BGRA frames are supported.";
  if (image->color_space() != gfx::ColorSpace::CreateSRGB() ||
      frame->ColorSpace() != image->color_space())
    return "Only matching sRGB native frames are supported.";
  if (image->surface_origin() != kTopLeft_GrSurfaceOrigin ||
      frame->coded_size() != image->size() ||
      frame->natural_size() != frame->visible_rect().size())
    return "Native export requires top-left storage and unscaled dimensions.";
  if (frame->metadata().transformation &&
      (frame->metadata().transformation->rotation != media::VIDEO_ROTATION_0 ||
       frame->metadata().transformation->mirrored))
    return "Native export does not apply rotation or mirroring.";
  if (!image->usage().Has(gpu::SHARED_IMAGE_USAGE_GLES2_READ))
    return "The native frame must support GPU read access.";
  return {};
}

bool NativeVideoFrameExporter::Initialize() {
  if (failed_)
    return false;
  if (provider_)
    return !provider_->IsContextLost() && remote_.is_connected();
  auto* local = blink::WebLocalFrame::FrameForCurrentContext();
  auto* frame = local ? content::RenderFrame::FromWebFrame(local) : nullptr;
  if (!frame)
    return false;
  frame->GetRemoteAssociatedInterfaces()->GetInterface(&remote_);
  remote_.set_disconnect_handler(base::BindOnce(
      &NativeVideoFrameExporter::Fail, weak_factory_.GetWeakPtr(),
      "The native frame host disconnected."));
  blink::Platform::WebGLContextInfo info;
  provider_ = blink::Platform::Current()->CreateWebGLGraphicsContextProvider(
      false, true, blink::Platform::kWebGL2ContextType, blink::WebURL(), &info);
  if (!provider_ || !info.using_gpu_compositing ||
      !info.using_passthrough_command_decoder ||
      !provider_->BindToCurrentSequence() || !provider_->ContextGL() ||
      !provider_->ContextSupport() || provider_->IsContextLost()) {
    provider_.reset();
    return false;
  }
  provider_->SetLostContextCallback(base::BindRepeating(
      &NativeVideoFrameExporter::Lost, weak_factory_.GetWeakPtr()));
  return true;
}

v8::Local<v8::Promise> NativeVideoFrameExporter::Export(
    v8::Isolate* isolate,
    v8::Local<v8::Value> value) {
  gin_helper::Promise<std::string> promise(isolate);
  auto result = promise.GetHandle();
  auto* wrapped = blink::V8VideoFrame::ToWrappable(isolate, value);
  auto source = wrapped ? wrapped->frame() : nullptr;
  std::string error = wrapped ? Validate(source) : "Expected a VideoFrame.";
  if (promise_)
    error = "A native VideoFrame export is already pending.";
  if (error.empty() && !Initialize())
    error =
        "A live hardware GLES context and Electron frame host are required.";
  if (!error.empty()) {
    promise.RejectWithErrorMessage(error);
    return result;
  }
  source_ = std::move(source);
  handle_ = source_->shared_image()->CloneGpuMemoryBufferHandle();
  info_ = mojom::NativeVideoFrameInfo::New(
      source_->shared_image()->format() == viz::SinglePlaneFormat::kRGBA_8888,
      source_->coded_size(), source_->visible_rect(),
      source_->timestamp().InMicroseconds());
  promise_.emplace(std::move(promise));
  auto* gl = provider_->ContextGL();
  texture_ = source_->shared_image()->CreateGLTexture(gl);
  if (!texture_) {
    Fail("Unable to acquire the source native texture.");
    return result;
  }
  // Acquire the actual backing, then wait for hardware completion. A plain
  // SignalSyncToken only orders service work and is not a native GPU fence.
  gl->GenQueriesEXT(1, &query_);
  gl->BeginQueryEXT(GL_COMMANDS_COMPLETED_CHROMIUM, query_);
  auto access = texture_->BeginAccess(source_->acquire_sync_token(), true);
  auto token =
      gpu::SharedImageTexture::ScopedAccess::EndAccess(std::move(access));
  media::WaitAndReplaceSyncTokenClient release_client(
      gl, base::BindOnce([](gpu::SyncToken value) { return value; }, token));
  source_->UpdateReleaseSyncToken(&release_client);
  source_->shared_image()->UpdateDestructionSyncToken(token);
  gl->EndQueryEXT(GL_COMMANDS_COMPLETED_CHROMIUM);
  gl->Flush();
  deadline_.Start(
      FROM_HERE, base::Seconds(15),
      base::BindOnce(
          &NativeVideoFrameExporter::Fail, weak_factory_.GetWeakPtr(),
          "Native frame GPU completion or handle transfer timed out."));
  provider_->ContextSupport()->SignalQuery(
      query_, base::BindOnce(&NativeVideoFrameExporter::Complete,
                             weak_factory_.GetWeakPtr()));
  return result;
}

void NativeVideoFrameExporter::Complete() {
  if (!promise_)
    return;
  auto* gl = provider_->ContextGL();
  if (provider_->IsContextLost() || gl->GetError() != GL_NO_ERROR) {
    Fail("Native frame synchronization failed or its GPU context was lost.");
    return;
  }
  gl->DeleteQueriesEXT(1, &query_);
  query_ = 0;
  texture_.reset();
  mojo::PendingRemote<mojom::NativeVideoFrameLease> lease;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SourceLease>(std::move(source_)),
                              lease.InitWithNewPipeAndPassReceiver());
  remote_->Export(std::move(handle_), std::move(info_), std::move(lease),
                  base::BindOnce(&NativeVideoFrameExporter::Settle,
                                 weak_factory_.GetWeakPtr()));
}

void NativeVideoFrameExporter::Lost() {
  failed_ = true;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&NativeVideoFrameExporter::Fail,
                     weak_factory_.GetWeakPtr(),
                     "Native frame synchronization context was lost."));
}

void NativeVideoFrameExporter::Fail(std::string error) {
  failed_ = true;
  Settle("", std::move(error));
}

void NativeVideoFrameExporter::Settle(const std::string& id,
                                      const std::string& error) {
  if (!promise_)
    return;
  deadline_.Stop();
  if (query_) {
    provider_->ContextGL()->DeleteQueriesEXT(1, &query_);
    query_ = 0;
  }
  texture_.reset();
  source_.reset();
  handle_ = gfx::GpuMemoryBufferHandle();
  info_.reset();
  auto promise = std::move(*promise_);
  promise_.reset();
  v8::HandleScope scope(promise.isolate());
  if (promise.GetContext().IsEmpty()) {
    ReleaseExport(id);
    return;
  }
  if (error.empty())
    promise.Resolve(id);
  else
    promise.RejectWithErrorMessage(error);
}
}  // namespace

void InstallNativeVideoFrameExport(v8::Isolate* isolate,
                                   v8::Local<v8::Object> exports) {
  auto exporter = base::MakeRefCounted<NativeVideoFrameExporter>();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod(
      "getNativeVideoFrameHandle",
      base::BindRepeating(&NativeVideoFrameExporter::Export, exporter));
  dict.SetMethod(
      "releaseNativeVideoFrameHandle",
      base::BindRepeating(&NativeVideoFrameExporter::ReleaseExport, exporter));
}
}  // namespace electron
