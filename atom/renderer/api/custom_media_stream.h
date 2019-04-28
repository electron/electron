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

}  // namespace CustomMediaStream

#endif
