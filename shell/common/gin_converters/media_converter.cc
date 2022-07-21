// Copyright (c) 2021 Slack Technologies, LLC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/media_converter.h"

#include <string>
#include <utility>

#include "content/public/browser/media_stream_request.h"
#include "content/public/browser/render_frame_host.h"
#include "gin/data_object_builder.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom.h"

namespace gin {

v8::Local<v8::Value> Converter<content::MediaStreamRequest>::ToV8(
    v8::Isolate* isolate,
    const content::MediaStreamRequest& request) {
  content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
      request.render_process_id, request.render_frame_id);
  return gin::DataObjectBuilder(isolate)
      .Set("frame", rfh)
      .Set("securityOrigin", request.security_origin)
      .Set("userGesture", request.user_gesture)
      .Set("videoRequested",
           request.video_type != blink::mojom::MediaStreamType::NO_SERVICE)
      .Set("audioRequested",
           request.audio_type != blink::mojom::MediaStreamType::NO_SERVICE)
      .Build();
}

bool Converter<blink::MediaStreamDevice>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    blink::MediaStreamDevice* out) {
  gin_helper::Dictionary dict;
  if (!gin::ConvertFromV8(isolate, val, &dict))
    return false;
  blink::mojom::MediaStreamType type;
  if (!dict.Get("type", &type))
    return false;
  std::string id;
  if (!dict.Get("id", &id))
    return false;
  std::string name;
  if (!dict.Get("name", &name))
    return false;
  *out = blink::MediaStreamDevice(type, id, name);
  return true;
}

v8::Local<v8::Value> Converter<blink::MediaStreamDevice>::ToV8(
    v8::Isolate* isolate,
    const blink::MediaStreamDevice& device) {
  return gin::DataObjectBuilder(isolate)
      .Set("type", device.type)
      .Set("id", device.id)
      .Set("name", device.name)
      .Build();
}

bool Converter<blink::mojom::StreamDevicesPtr>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    blink::mojom::StreamDevicesPtr* out) {
  gin_helper::Dictionary dict;
  if (!gin::ConvertFromV8(isolate, val, &dict))
    return false;
  *out = blink::mojom::StreamDevices::New();
  if (dict.Has("audioDevice")) {
    blink::MediaStreamDevice audio_device;
    if (!dict.Get("audioDevice", &audio_device))
      return false;
    (*out)->audio_device = std::move(audio_device);
  }
  if (dict.Has("videoDevice")) {
    blink::MediaStreamDevice video_device;
    if (!dict.Get("videoDevice", &video_device))
      return false;
    (*out)->video_device = std::move(video_device);
  }
  return true;
}

bool Converter<blink::mojom::MediaStreamRequestResult>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    blink::mojom::MediaStreamRequestResult* out) {
  std::string type;
  if (!gin::ConvertFromV8(isolate, val, &type))
    return false;
  if (type == "ok") {
    *out = blink::mojom::MediaStreamRequestResult::OK;
    return true;
  }
  if (type == "permissionDenied") {
    *out = blink::mojom::MediaStreamRequestResult::PERMISSION_DENIED;
    return true;
  }
  if (type == "permissionDismissed") {
    *out = blink::mojom::MediaStreamRequestResult::PERMISSION_DISMISSED;
    return true;
  }
  if (type == "invalidState") {
    *out = blink::mojom::MediaStreamRequestResult::INVALID_STATE;
    return true;
  }
  if (type == "noHardware") {
    *out = blink::mojom::MediaStreamRequestResult::NO_HARDWARE;
    return true;
  }
  if (type == "invalidSecurityOrigin") {
    *out = blink::mojom::MediaStreamRequestResult::INVALID_SECURITY_ORIGIN;
    return true;
  }
  if (type == "tabCaptureFailure") {
    *out = blink::mojom::MediaStreamRequestResult::TAB_CAPTURE_FAILURE;
    return true;
  }
  if (type == "screenCaptureFailure") {
    *out = blink::mojom::MediaStreamRequestResult::SCREEN_CAPTURE_FAILURE;
    return true;
  }
  if (type == "captureFailure") {
    *out = blink::mojom::MediaStreamRequestResult::CAPTURE_FAILURE;
    return true;
  }
  if (type == "constraintNotSatisfied") {
    *out = blink::mojom::MediaStreamRequestResult::CONSTRAINT_NOT_SATISFIED;
    return true;
  }
  if (type == "trackStartFailureAudio") {
    *out = blink::mojom::MediaStreamRequestResult::TRACK_START_FAILURE_AUDIO;
    return true;
  }
  if (type == "trackStartFailureVideo") {
    *out = blink::mojom::MediaStreamRequestResult::TRACK_START_FAILURE_VIDEO;
    return true;
  }
  if (type == "notSupported") {
    *out = blink::mojom::MediaStreamRequestResult::NOT_SUPPORTED;
    return true;
  }
  if (type == "failedDueToShutdown") {
    *out = blink::mojom::MediaStreamRequestResult::FAILED_DUE_TO_SHUTDOWN;
    return true;
  }
  if (type == "killSwitchOn") {
    *out = blink::mojom::MediaStreamRequestResult::KILL_SWITCH_ON;
    return true;
  }
  if (type == "systemPermissionDenied") {
    *out = blink::mojom::MediaStreamRequestResult::SYSTEM_PERMISSION_DENIED;
    return true;
  }
  if (type == "deviceInUse") {
    *out = blink::mojom::MediaStreamRequestResult::DEVICE_IN_USE;
    return true;
  }
  return false;
}

v8::Local<v8::Value> Converter<blink::MediaStreamRequestType>::ToV8(
    v8::Isolate* isolate,
    const blink::MediaStreamRequestType& request_type) {
  switch (request_type) {
    case blink::MEDIA_GET_OPEN_DEVICE:
      return gin::StringToV8(isolate, "getOpenDevice");
    case blink::MEDIA_DEVICE_ACCESS:
      return gin::StringToV8(isolate, "deviceAccess");
    case blink::MEDIA_DEVICE_UPDATE:
      return gin::StringToV8(isolate, "deviceUpdate");
    case blink::MEDIA_GENERATE_STREAM:
      return gin::StringToV8(isolate, "generateStream");
    case blink::MEDIA_OPEN_DEVICE_PEPPER_ONLY:
      return gin::StringToV8(isolate, "openDevicePepperOnly");
  }
}

v8::Local<v8::Value> Converter<blink::mojom::MediaStreamType>::ToV8(
    v8::Isolate* isolate,
    const blink::mojom::MediaStreamType& stream_type) {
  switch (stream_type) {
    case blink::mojom::MediaStreamType::NO_SERVICE:
      return gin::StringToV8(isolate, "noService");
    case blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE:
      return gin::StringToV8(isolate, "deviceAudioCapture");
    case blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE:
      return gin::StringToV8(isolate, "deviceVideoCapture");
    case blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE:
      return gin::StringToV8(isolate, "gumTabAudioCapture");
    case blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE:
      return gin::StringToV8(isolate, "gumTabVideoCapture");
    case blink::mojom::MediaStreamType::GUM_DESKTOP_VIDEO_CAPTURE:
      return gin::StringToV8(isolate, "gumDesktopVideoCapture");
    case blink::mojom::MediaStreamType::GUM_DESKTOP_AUDIO_CAPTURE:
      return gin::StringToV8(isolate, "gumDesktopAudioCapture");
    case blink::mojom::MediaStreamType::DISPLAY_VIDEO_CAPTURE:
      return gin::StringToV8(isolate, "displayVideoCapture");
    case blink::mojom::MediaStreamType::DISPLAY_VIDEO_CAPTURE_SET:
      return gin::StringToV8(isolate, "displayVideoCaptureSet");
    case blink::mojom::MediaStreamType::DISPLAY_AUDIO_CAPTURE:
      return gin::StringToV8(isolate, "displayAudioCapture");
    case blink::mojom::MediaStreamType::DISPLAY_VIDEO_CAPTURE_THIS_TAB:
      return gin::StringToV8(isolate, "displayVideoCaptureThisTab");
    case blink::mojom::MediaStreamType::NUM_MEDIA_TYPES:
      NOTREACHED();
      return v8::Null(isolate);
  }
}

bool Converter<blink::mojom::MediaStreamType>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    blink::mojom::MediaStreamType* out) {
  std::string type;
  if (!gin::ConvertFromV8(isolate, val, &type))
    return false;
  if (type == "noService") {
    *out = blink::mojom::MediaStreamType::NO_SERVICE;
    return true;
  }
  if (type == "deviceAudioCapture") {
    *out = blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE;
    return true;
  }
  if (type == "deviceVideoCapture") {
    *out = blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE;
    return true;
  }
  if (type == "gumTabAudioCapture") {
    *out = blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE;
    return true;
  }
  if (type == "gumTabVideoCapture") {
    *out = blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE;
    return true;
  }
  if (type == "gumDesktopVideoCapture") {
    *out = blink::mojom::MediaStreamType::GUM_DESKTOP_VIDEO_CAPTURE;
    return true;
  }
  if (type == "gumDesktopAudioCapture") {
    *out = blink::mojom::MediaStreamType::GUM_DESKTOP_AUDIO_CAPTURE;
    return true;
  }
  if (type == "displayVideoCapture") {
    *out = blink::mojom::MediaStreamType::DISPLAY_VIDEO_CAPTURE;
    return true;
  }
  if (type == "displayAudioCapture") {
    *out = blink::mojom::MediaStreamType::DISPLAY_AUDIO_CAPTURE;
    return true;
  }
  if (type == "displayVideoCaptureThisTab") {
    *out = blink::mojom::MediaStreamType::DISPLAY_VIDEO_CAPTURE_THIS_TAB;
    return true;
  }
  return false;
}

}  // namespace gin
