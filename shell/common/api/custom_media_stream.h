// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_API_CUSTOM_MEDIA_STREAM_H_
#define SHELL_COMMON_API_CUSTOM_MEDIA_STREAM_H_

#include <v8.h>
#include <memory>

namespace CustomMediaStream {

// TODO: Make nested to VideoFrame
enum class Plane { Y, U, V };

// TODO: Make nested to VideoFrame
struct Format {
  unsigned width;
  unsigned height;
};

// Frame interface for non-GC frames
// (When accessing the API from C++)
// TODO: Refactor to a class
struct VideoFrame {
  virtual Format format() = 0;
  virtual unsigned stride(Plane) = 0;
  virtual unsigned rows(Plane) = 0;
  virtual void* data(Plane plane) = 0;
};

struct Timestamp {
  double milliseconds;
};

// Control object interface for non-GC frames
// (When accessing the API from C++)
// TODO: Rename to VideoFrameControl ???
struct VideoFrameCallback {
  virtual VideoFrame* allocateFrame(Timestamp ts,
                                    Format const* format = nullptr) = 0;
  virtual void queueFrame(Timestamp ts, VideoFrame* frame) = 0;
  virtual void releaseFrame(VideoFrame* frame) = 0;
};

// Retrieves VideoFrameCallback pointer from an internal field of an object
// TODO: Why inline? move implementation to .cc
inline VideoFrameCallback* unwrapCallback(v8::Local<v8::Value> value) {
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

// Custom deleter for VideoFrame
// TODO: Refactor to a class, groom, rename to VideoFrameDeleter
// TODO: Rename callback_ to holder_
struct VideoFrameReleaser {
  void operator()(VideoFrame* frame) const;
  // private:
  friend VideoFrameCallbackHolder;
  explicit VideoFrameReleaser(VideoFrameCallbackHolderPtr);
  VideoFrameReleaser(VideoFrameReleaser const&);
  ~VideoFrameReleaser();

  VideoFrameCallbackHolderPtr callback_;
};

// TODO: Why inline? move implementation to .cc
inline VideoFrameReleaser::VideoFrameReleaser(VideoFrameCallbackHolderPtr cb)
    : callback_(cb) {}
inline VideoFrameReleaser::VideoFrameReleaser(VideoFrameReleaser const&) =
    default;
inline VideoFrameReleaser::~VideoFrameReleaser() = default;

}  // namespace detail

// Helper object that wraps VideoFrameCallback functionality
// Simplifies allocation and deletion of non-GC frames
// in a safe manner.
// Holds a reference to the VideoFrameCallback
// TODO: Refactor to a class, groom, make non-copyable
struct VideoFrameCallbackHolder
    : std::enable_shared_from_this<VideoFrameCallbackHolder> {
  using FramePtr = std::unique_ptr<VideoFrame, detail::VideoFrameReleaser>;

  FramePtr allocate(Timestamp ts, Format const* format) {
    return {callback_->allocateFrame(ts, format),
            detail::VideoFrameReleaser(shared_from_this())};
  }

  // TODO: Move implementations to .cc
  FramePtr allocate(Timestamp ts, Format const& f) { return allocate(ts, &f); }
  FramePtr allocate(Timestamp ts) { return allocate(ts, nullptr); }

  void queue(Timestamp ts, FramePtr ptr) {
    callback_->queueFrame(ts, ptr.release());
  }

  // Creates the holder based on VideoFrameCallback object
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

// TODO: Why are these inline? groom, move to .cc
inline VideoFrameCallbackHolder::VideoFrameCallbackHolder(
    v8::Isolate* isolate,
    v8::Local<v8::Object> wrapper,
    VideoFrameCallback* cb)
    : wrapper_(isolate, wrapper), callback_(cb) {}

inline VideoFrameCallbackHolder::~VideoFrameCallbackHolder() = default;

namespace detail {
// TODO: WHy inline? move to .cc
inline void VideoFrameReleaser::operator()(VideoFrame* frame) const {
  callback_->callback_->releaseFrame(frame);
}
}  // namespace detail

}  // namespace CustomMediaStream

#endif  // SHELL_COMMON_API_CUSTOM_MEDIA_STREAM_H_
