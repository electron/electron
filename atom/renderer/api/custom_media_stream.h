#ifndef __CUSTOM_MEDIA_STREAM_H__
#define __CUSTOM_MEDIA_STREAM_H__

namespace CustomMediaStream {

enum class Plane { Y, U, V };

struct Format {
  unsigned width;
  unsigned height;
};

struct VideoFrame {
  virtual Format format() = 0;
  virtual unsigned stride(Plane) = 0;
  virtual unsigned rows(Plane) = 0;
  virtual void* data(Plane plane) = 0;
};

struct Timestamp {
  double milliseconds;
};

struct VideoFrameCallback {
  virtual VideoFrame* allocateFrame(Timestamp ts,
                                    Format const* format = nullptr) = 0;
  virtual void queueFrame(Timestamp ts, VideoFrame* frame) = 0;
  virtual void releaseFrame(VideoFrame* frame) = 0;
};

inline VideoFrameCallback* unwrapCallback(v8::Handle<v8::Value> value) {
  if (!value->IsObject())
    return nullptr;
  v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(value);
  if (obj->InternalFieldCount() != 2)
    return nullptr;
  return static_cast<VideoFrameCallback*>(
      obj->GetAlignedPointerFromInternalField(1));
}

struct VideoFrameCallbackHolder;

namespace detail {

using VideoFrameCallbackHolderPtr = std::shared_ptr<VideoFrameCallbackHolder>;

struct VideoFrameReleaser {
  void operator()(VideoFrame* frame) const;
  // private:
  friend VideoFrameCallbackHolder;
  VideoFrameReleaser(std::shared_ptr<VideoFrameCallbackHolder>);
  VideoFrameReleaser(VideoFrameReleaser const&);
  ~VideoFrameReleaser();

  VideoFrameCallbackHolderPtr callback_;
};

inline VideoFrameReleaser::VideoFrameReleaser(
    std::shared_ptr<VideoFrameCallbackHolder> cb)
    : callback_(cb) {}
inline VideoFrameReleaser::VideoFrameReleaser(VideoFrameReleaser const&) =
    default;
inline VideoFrameReleaser::~VideoFrameReleaser() = default;

}  // namespace detail

struct VideoFrameCallbackHolder
    : std::enable_shared_from_this<VideoFrameCallbackHolder> {
  using FramePtr = std::unique_ptr<VideoFrame, detail::VideoFrameReleaser>;

  FramePtr allocate(Timestamp ts, Format const* format) {
    return {callback_->allocateFrame(ts, format), shared_from_this()};
  }

  FramePtr allocate(Timestamp ts, Format const& f) { return allocate(ts, &f); }
  FramePtr allocate(Timestamp ts) { return allocate(ts, nullptr); }

  void queue(Timestamp ts, FramePtr ptr) {
    callback_->queueFrame(ts, ptr.release());
  }

  static std::shared_ptr<VideoFrameCallbackHolder> unwrap(
      v8::Isolate* isolate,
      v8::Local<v8::Value> value) {
    auto* cb = unwrapCallback(value);
    if (cb) {
      return std::make_shared<VideoFrameCallbackHolder>(
          isolate, v8::Local<v8::Object>::Cast(value), cb);
    }
    return nullptr;
  }

  VideoFrameCallbackHolder(v8::Isolate* isolate,
                           v8::Local<v8::Object> wrapper,
                           VideoFrameCallback* cb);
  ~VideoFrameCallbackHolder();

 private:
  friend detail::VideoFrameReleaser;
  v8::Global<v8::Object> wrapper_;
  VideoFrameCallback* callback_;
};

inline VideoFrameCallbackHolder::VideoFrameCallbackHolder(
    v8::Isolate* isolate,
    v8::Local<v8::Object> wrapper,
    VideoFrameCallback* cb)
    : wrapper_(isolate, wrapper), callback_(cb) {}

inline VideoFrameCallbackHolder::~VideoFrameCallbackHolder() = default;

namespace detail {
inline void VideoFrameReleaser::operator()(VideoFrame* frame) const {
  callback_->callback_->releaseFrame(frame);
}
}  // namespace detail

}  // namespace CustomMediaStream

#endif
