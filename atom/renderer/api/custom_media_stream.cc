#include <iostream>
#include <memory>
#include <utility>

#include <base/base64.h>
#include <base/rand_util.h>
#include <base/strings/utf_string_conversions.h>

#define INSIDE_BLINK 1

#include <content/public/renderer/render_thread.h>
#include <content/renderer/media/stream/media_stream_video_capturer_source.h>
#include <content/renderer/media/stream/media_stream_video_source.h>
#include <content/renderer/media/stream/media_stream_video_track.h>
#include <media/base/video_frame.h>
#include <media/base/video_frame_pool.h>
#include <media/capture/video_capturer_source.h>
#include <third_party/blink/public/platform/web_media_stream_source.h>
#include <third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h>
#include <third_party/blink/renderer/modules/mediastream/media_stream_track.h>
#include <third_party/blink/renderer/platform/bindings/to_v8.h>

#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/node_includes.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "native_mate/wrappable.h"

namespace mate {
template <>
struct Converter<blink::MediaStreamComponent*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   blink::MediaStreamComponent* track) {
    return blink::ToV8(
        blink::MediaStreamTrack::Create(
            blink::ToExecutionContext(isolate->GetCurrentContext()), track),
        isolate->GetCurrentContext()->Global(), isolate);
  }
};

template <>
struct Converter<v8::Local<v8::ArrayBuffer>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   v8::Local<v8::ArrayBuffer> v) {
    return v;
  }
};

template <>
struct Converter<base::TimeDelta> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, base::TimeDelta v) {
    return v8::Number::New(isolate, v.InMillisecondsF());
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::TimeDelta* out) {
    double d;
    if (Converter<double>::FromV8(isolate, val, &d)) {
      *out = base::TimeDelta::FromMillisecondsD(d);
      return true;
    }

    return false;
  }
};

template <>
struct Converter<base::TimeTicks> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, base::TimeTicks v) {
    return v8::Number::New(
        isolate, (v - base::TimeTicks::UnixEpoch()).InMillisecondsF());
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::TimeTicks* out) {
    base::TimeDelta d;
    if (Converter<base::TimeDelta>::FromV8(isolate, val, &d)) {
      *out = base::TimeTicks::UnixEpoch() + d;
      return true;
    }
    return false;
  }
};
}  // namespace mate

namespace {

void test(mate::Arguments* args) {
  std::cout << "test" << std::endl;
}

struct Frame : mate::Wrappable<Frame> {
  Frame(v8::Isolate* isolate, scoped_refptr<media::VideoFrame> frame)
      : frame_(std::move(frame)) {
    Init(isolate);
  }

  std::uint32_t width() {
    if (!frame_)
      return 0;
    return frame_->visible_rect().width();
  }

  std::uint32_t height() {
    if (!frame_)
      return 0;
    return frame_->visible_rect().height();
  }

  std::uint32_t stride(int plane) {
    if (!frame_)
      return 0;

    switch (plane) {
      case media::VideoFrame::kYPlane:
      case media::VideoFrame::kUPlane:
      case media::VideoFrame::kVPlane:
        return frame_->stride(plane);
      default:
        return -1;
    }
  }

  base::TimeDelta timestamp() {
    if (!frame_)
      return {};

    return frame_->timestamp();
  }

  v8::Local<v8::ArrayBuffer> data(int plane) {
    if (!frame_)
      return {};

    switch (plane) {
      case media::VideoFrame::kYPlane:
      case media::VideoFrame::kUPlane:
      case media::VideoFrame::kVPlane:
        return v8::ArrayBuffer::New(
            isolate(), frame_->visible_data(plane),
            frame_->stride(plane) * frame_->rows(plane));
      default:
        return {};
    }
  }

  v8::Local<v8::ArrayBuffer> y() { return data(media::VideoFrame::kYPlane); }

  v8::Local<v8::ArrayBuffer> u() { return data(media::VideoFrame::kUPlane); }

  v8::Local<v8::ArrayBuffer> v() { return data(media::VideoFrame::kVPlane); }

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype) {
    prototype->SetClassName(
        mate::StringToV8(isolate, "CustomMediaStreamVideoFrame"));
    mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
        .SetMethod("data", &Frame::data)
        .SetMethod("stride", &Frame::stride)
        .SetProperty("width", &Frame::width)
        .SetProperty("height", &Frame::height)
        .SetProperty("timestamp", &Frame::timestamp)
        .SetProperty("y", &Frame::y)
        .SetProperty("u", &Frame::u)
        .SetProperty("v", &Frame::v);
  }

  scoped_refptr<media::VideoFrame> frame_;
};

struct ControlObject : mate::Wrappable<ControlObject> {
  ControlObject(v8::Isolate* isolate) { Init(isolate); }

  Frame* allocate(gfx::Size size, base::TimeDelta timestamp) {
    auto f = framePool_.CreateFrame(media::PIXEL_FORMAT_I420, size,
                                    gfx::Rect(size), size, timestamp);
    return new Frame(isolate(), f);
  }

  void queue(Frame* frame, base::TimeTicks timestamp) {
    io_task_runner_->PostTask(FROM_HERE,
                              base::Bind(deliver_, frame->frame_, timestamp));
    frame->frame_ = nullptr;
  }

  static mate::Handle<ControlObject> create(v8::Isolate* isolate) {
    return mate::CreateHandle(isolate, new ControlObject(isolate));
  }

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype) {
    prototype->SetClassName(
        mate::StringToV8(isolate, "CustomMediaStreamController"));
    mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
        .SetMethod("allocateFrame", &ControlObject::allocate)
        .SetMethod("queueFrame", &ControlObject::queue);
  }

  content::VideoCaptureDeliverFrameCB deliver_;
  media::VideoFramePool framePool_;
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_ =
      content::RenderThread::Get()->GetIOTaskRunner();
};

struct CustomCapturerSource : media::VideoCapturerSource {
  CustomCapturerSource(
      v8::Isolate* isolate,
      gfx::Size resolution,
      std::size_t framerate,
      base::RepeatingCallback<void(ControlObject*)> onStartCapture,
      base::RepeatingCallback<void()> onStopCapture)
      : format_(resolution, framerate, media::PIXEL_FORMAT_I420),
        onStartCapture_(std::move(onStartCapture)),
        onStopCapture_(std::move(onStopCapture)) {
    auto handle = ControlObject::create(isolate);
    control_wrapper_ = v8::Global<v8::Value>(isolate, handle.ToV8());
    control_ = handle.get();
  }

  media::VideoCaptureFormats GetPreferredFormats() override {
    return {format_};
  }

  void StartCapture(const media::VideoCaptureParams& params,
                    const VideoCaptureDeliverFrameCB& frame_callback,
                    const RunningCallback& running_callback) override {
    std::cerr << "StartCapture"
              << media::VideoCaptureFormat::ToString(params.requested_format)
              << std::endl;

    control_->deliver_ = frame_callback;
    running_callback.Run(true);
  }

  void StopCapture() override { std::cerr << "StopCapture" << std::endl; }

  void Resume() override {
    std::cerr << "Resume" << std::endl;
    onStartCapture_.Run(control_);
  }

  void MaybeSuspend() override {
    std::cerr << "MaybeSuspend" << std::endl;
    onStopCapture_.Run();
  }

  v8::Global<v8::Value> control_wrapper_;
  ControlObject* control_;
  media::VideoCaptureFormat format_;
  base::RepeatingCallback<void(ControlObject*)> onStartCapture_;
  base::RepeatingCallback<void()> onStopCapture_;
};

blink::MediaStreamComponent* createTrack(
    v8::Isolate* isolate,
    gfx::Size resolution,
    int framerate,
    base::RepeatingCallback<void(ControlObject*)> onStartCapture,
    base::RepeatingCallback<void()> onStopCapture) {
  std::unique_ptr<CustomCapturerSource> source{new CustomCapturerSource(
      isolate, resolution, framerate, onStartCapture, onStopCapture)};

  std::string str_track_id;
  base::Base64Encode(base::RandBytesAsString(64), &str_track_id);
  const blink::WebString track_id = blink::WebString::FromASCII(str_track_id);

  std::unique_ptr<content::MediaStreamVideoSource> platform_source(
      new content::MediaStreamVideoCapturerSource(
          content::MediaStreamVideoSource::SourceStoppedCallback(),
          std::move(source)));

  content::MediaStreamVideoSource* t = platform_source.get();
  blink::WebMediaStreamSource webkit_source;
  webkit_source.Initialize(track_id, blink::WebMediaStreamSource::kTypeVideo,
                           track_id, false);
  webkit_source.SetPlatformSource(std::move(platform_source));

  std::unique_ptr<content::MediaStreamVideoTrack> platform_track{
      new content::MediaStreamVideoTrack(
          t, content::MediaStreamVideoSource::ConstraintsCallback(), true)};

  blink::WebMediaStreamTrack track;
  track.Initialize(webkit_source);
  track.SetPlatformTrack(std::move(platform_track));

  return track;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  std::cout << "Initialize" << std::endl;
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("test", &test);
  dict.SetMethod("createTrack", &createTrack);
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_renderer_custom_media_stream, Initialize)
