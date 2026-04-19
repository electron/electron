// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/media/system_audio_capturer.h"

#import <ApplicationServices/ApplicationServices.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreMedia/CoreMedia.h>
#import <Foundation/Foundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/raw_ptr.h"

namespace electron {
class SystemAudioCapturer;
}  // namespace electron

namespace {

struct PlatformState {
  dispatch_queue_t queue;
  dispatch_semaphore_t start_semaphore;
  dispatch_semaphore_t stop_semaphore;
  id stream;
  id output_handler;
  std::atomic<bool> started;
  std::atomic<bool> start_failed;

  PlatformState()
      : queue(dispatch_queue_create("electron.system_audio_capture",
                                    DISPATCH_QUEUE_SERIAL)),
        start_semaphore(dispatch_semaphore_create(0)),
        stop_semaphore(dispatch_semaphore_create(0)),
        stream(nil),
        output_handler(nil),
        started(false),
        start_failed(false) {}
};

}  // namespace

@interface SCKAudioOutputHandler : NSObject <SCStreamOutput>
- (instancetype)initWithCapturer:(electron::SystemAudioCapturer*)capturer;
@end

@implementation SCKAudioOutputHandler {
  raw_ptr<electron::SystemAudioCapturer> capturer_;
}

- (instancetype)initWithCapturer:(electron::SystemAudioCapturer*)capturer {
  self = [super init];
  if (self) {
    capturer_ = capturer;
  }
  return self;
}

// SAFETY: ScreenCaptureKit and CoreMedia expose C APIs that require raw
// pointer access and AudioBufferList indexing.
#pragma clang unsafe_buffer_usage begin
- (void)stream:(SCStream*)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
                   ofType:(SCStreamOutputType)type API_AVAILABLE(macos(13.0)) {
  if (type != SCStreamOutputTypeAudio)
    return;

  electron::SystemAudioCapturer* capturer = capturer_;

  if (sampleBuffer == nullptr || !CMSampleBufferIsValid(sampleBuffer) ||
      !CMSampleBufferDataIsReady(sampleBuffer)) {
    capturer->OnPlatformError("Invalid ScreenCaptureKit audio sample buffer.");
    return;
  }

  CMAudioFormatDescriptionRef format_desc =
      CMSampleBufferGetFormatDescription(sampleBuffer);
  if (format_desc == nullptr) {
    capturer->OnPlatformError(
        "Missing ScreenCaptureKit audio format description.");
    return;
  }

  const AudioStreamBasicDescription* asbd =
      CMAudioFormatDescriptionGetStreamBasicDescription(format_desc);
  if (asbd == nullptr) {
    capturer->OnPlatformError("Unable to read audio stream basic description.");
    return;
  }

  const uint32_t channels = static_cast<uint32_t>(asbd->mChannelsPerFrame);
  if (channels == 0) {
    capturer->OnPlatformError("ScreenCaptureKit reported zero audio channels.");
    return;
  }

  size_t frames = static_cast<size_t>(CMSampleBufferGetNumSamples(sampleBuffer));
  const bool is_float = (asbd->mFormatFlags & kAudioFormatFlagIsFloat) != 0;
  const bool is_signed_int =
      (asbd->mFormatFlags & kAudioFormatFlagIsSignedInteger) != 0;
  const bool is_non_interleaved =
      (asbd->mFormatFlags & kAudioFormatFlagIsNonInterleaved) != 0;

  size_t bytes_per_sample = 0;
  if (is_float && asbd->mBitsPerChannel == 32) {
    bytes_per_sample = sizeof(float);
  } else if (is_signed_int && asbd->mBitsPerChannel == 16) {
    bytes_per_sample = sizeof(int16_t);
  } else if (is_signed_int && asbd->mBitsPerChannel == 32) {
    bytes_per_sample = sizeof(int32_t);
  }

  if (bytes_per_sample == 0) {
    capturer->OnPlatformError("Unsupported ScreenCaptureKit audio sample format.");
    return;
  }

  const auto emit_converted_pcm = [&](std::vector<float> pcm) {
    CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    const double timestamp_ms =
        CMTIME_IS_VALID(pts) ? (CMTimeGetSeconds(pts) * 1000.0) : 0.0;

    capturer->OnPlatformPcmData(
        std::move(pcm), static_cast<uint32_t>(asbd->mSampleRate),
        static_cast<uint16_t>(channels), timestamp_ms,
        /*from_system_capture=*/true);
  };

  if (is_non_interleaved) {
    if (frames == 0) {
      capturer->OnPlatformError(
          "ScreenCaptureKit reported empty non-interleaved audio frame.");
      return;
    }

    std::vector<uint8_t> abl_storage(
        sizeof(AudioBufferList) + sizeof(AudioBuffer) * (channels - 1));
    AudioBufferList* buffer_list =
        reinterpret_cast<AudioBufferList*>(abl_storage.data());
    buffer_list->mNumberBuffers = channels;

    std::vector<std::vector<uint8_t>> channel_raw(channels);
    for (uint32_t ch = 0; ch < channels; ++ch) {
      channel_raw[ch].resize(frames * bytes_per_sample);
      buffer_list->mBuffers[ch].mNumberChannels = 1;
      buffer_list->mBuffers[ch].mDataByteSize =
          static_cast<uint32_t>(channel_raw[ch].size());
      buffer_list->mBuffers[ch].mData = channel_raw[ch].data();
    }

    OSStatus copy_pcm_status = CMSampleBufferCopyPCMDataIntoAudioBufferList(
        sampleBuffer, 0, static_cast<int32_t>(frames), buffer_list);

    if (copy_pcm_status != noErr) {
      capturer->OnPlatformError(
          "Failed copying non-interleaved PCM data from sample buffer.");
      return;
    }

    std::vector<float> pcm(frames * channels, 0.0f);
    if (is_float) {
      for (uint32_t ch = 0; ch < channels; ++ch) {
        const float* src = reinterpret_cast<const float*>(channel_raw[ch].data());
        for (size_t frame = 0; frame < frames; ++frame)
          pcm[frame * channels + ch] = src[frame];
      }
    } else if (asbd->mBitsPerChannel == 16) {
      for (uint32_t ch = 0; ch < channels; ++ch) {
        const int16_t* src =
            reinterpret_cast<const int16_t*>(channel_raw[ch].data());
        for (size_t frame = 0; frame < frames; ++frame)
          pcm[frame * channels + ch] = static_cast<float>(src[frame]) / 32768.0f;
      }
    } else {
      for (uint32_t ch = 0; ch < channels; ++ch) {
        const int32_t* src =
            reinterpret_cast<const int32_t*>(channel_raw[ch].data());
        for (size_t frame = 0; frame < frames; ++frame)
          pcm[frame * channels + ch] =
              static_cast<float>(src[frame]) / 2147483648.0f;
      }
    }

    emit_converted_pcm(std::move(pcm));
    return;
  }

  CMBlockBufferRef data_buffer = CMSampleBufferGetDataBuffer(sampleBuffer);
  if (data_buffer == nullptr) {
    capturer->OnPlatformError("Missing audio data buffer in sample buffer.");
    return;
  }

  const size_t data_bytes =
      static_cast<size_t>(CMBlockBufferGetDataLength(data_buffer));
  if (data_bytes == 0) {
    capturer->OnPlatformError("ScreenCaptureKit audio data buffer is empty.");
    return;
  }

  const size_t total_samples_from_buffer = data_bytes / bytes_per_sample;
  if (frames == 0)
    frames = total_samples_from_buffer / channels;

  if (frames == 0) {
    capturer->OnPlatformError(
        "ScreenCaptureKit audio frame resolved to zero samples.");
    return;
  }

  const size_t expected_samples = frames * channels;
  const size_t copy_samples = std::min(expected_samples, total_samples_from_buffer);

  std::vector<uint8_t> raw(copy_samples * bytes_per_sample);
  OSStatus copy_status =
      CMBlockBufferCopyDataBytes(data_buffer, 0, raw.size(), raw.data());
  if (copy_status != noErr) {
    capturer->OnPlatformError(
        "Failed copying interleaved PCM data from sample buffer.");
    return;
  }

  std::vector<float> pcm(expected_samples, 0.0f);
  if (is_float) {
    const float* src = reinterpret_cast<const float*>(raw.data());
    for (size_t i = 0; i < copy_samples; ++i)
      pcm[i] = src[i];
  } else if (asbd->mBitsPerChannel == 16) {
    const int16_t* src = reinterpret_cast<const int16_t*>(raw.data());
    for (size_t i = 0; i < copy_samples; ++i)
      pcm[i] = static_cast<float>(src[i]) / 32768.0f;
  } else {
    const int32_t* src = reinterpret_cast<const int32_t*>(raw.data());
    for (size_t i = 0; i < copy_samples; ++i)
      pcm[i] = static_cast<float>(src[i]) / 2147483648.0f;
  }

  emit_converted_pcm(std::move(pcm));
}
#pragma clang unsafe_buffer_usage end

@end

namespace {

bool StartPlatformCaptureMac13(electron::SystemAudioCapturer* capturer,
                               uintptr_t* platform_impl)
    API_AVAILABLE(macos(13.0)) {
  if (*platform_impl == 0) {
    *platform_impl = reinterpret_cast<uintptr_t>(new PlatformState());
  }

  auto* state = reinterpret_cast<PlatformState*>(*platform_impl);
  state->started.store(false);
  state->start_failed.store(false);

  __block PlatformState* state_ref = state;
  __block electron::SystemAudioCapturer* capturer_ref = capturer;

  [SCShareableContent
      getShareableContentWithCompletionHandler:^(SCShareableContent* content,
                                                 NSError* error) {
        if (error != nil || content == nil || content.displays.count == 0) {
          std::string message;
          if (error != nil) {
            message = [[error localizedDescription] UTF8String];
          } else {
            message = "No shareable display available for ScreenCaptureKit.";
          }
          capturer_ref->OnPlatformError(message);
          state_ref->start_failed.store(true);
          dispatch_semaphore_signal(state_ref->start_semaphore);
          return;
        }

        SCDisplay* display = content.displays.firstObject;
        SCContentFilter* filter =
            [[SCContentFilter alloc] initWithDisplay:display
                                    excludingWindows:@[]];

        SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
        config.capturesAudio = YES;
        config.sampleRate = 48000;
        config.channelCount = 2;
        config.excludesCurrentProcessAudio = NO;
        config.width = display.width;
        config.height = display.height;
        config.queueDepth = 5;
        config.minimumFrameInterval = CMTimeMake(1, 60);

        SCStream* stream = [[SCStream alloc] initWithFilter:filter
                                              configuration:config
                                                   delegate:nil];
        SCKAudioOutputHandler* handler =
            [[SCKAudioOutputHandler alloc] initWithCapturer:capturer_ref];

        NSError* add_output_error = nil;
        BOOL did_add = [stream addStreamOutput:handler
                                          type:SCStreamOutputTypeAudio
                            sampleHandlerQueue:state_ref->queue
                                          error:&add_output_error];

        if (!did_add || add_output_error != nil) {
          std::string message;
          if (add_output_error != nil) {
            message = [[add_output_error localizedDescription] UTF8String];
          } else {
            message = "Failed to add ScreenCaptureKit audio output.";
          }
          capturer_ref->OnPlatformError(message);
          state_ref->start_failed.store(true);
          dispatch_semaphore_signal(state_ref->start_semaphore);
          return;
        }

        state_ref->stream = stream;
        state_ref->output_handler = handler;

        [stream startCaptureWithCompletionHandler:^(NSError* start_error) {
          if (start_error != nil) {
            capturer_ref->OnPlatformError(
                [[start_error localizedDescription] UTF8String]);
            state_ref->start_failed.store(true);
          } else {
            state_ref->started.store(true);
          }
          dispatch_semaphore_signal(state_ref->start_semaphore);
        }];
      }];

  dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, 5LL * NSEC_PER_SEC);
  long wait_result = dispatch_semaphore_wait(state->start_semaphore, timeout);

  const bool succeeded =
      !(wait_result != 0 || state->start_failed.load() || !state->started.load());

  if (!succeeded && wait_result != 0) {
    capturer->OnPlatformError("Timed out waiting for ScreenCaptureKit stream start.");
  }

  return succeeded;
}

void StopPlatformCaptureMac13(uintptr_t* platform_impl)
    API_AVAILABLE(macos(13.0)) {
  auto* state = reinterpret_cast<PlatformState*>(*platform_impl);
  if (state == nullptr)
    return;

  SCStream* stream = static_cast<SCStream*>(state->stream);
  if (stream != nil) {
    __block PlatformState* state_ref = state;
    [stream stopCaptureWithCompletionHandler:^(NSError* error) {
      (void)error;
      dispatch_semaphore_signal(state_ref->stop_semaphore);
    }];

    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, 2LL * NSEC_PER_SEC);
    dispatch_semaphore_wait(state->stop_semaphore, timeout);

    state->stream = nil;
    state->output_handler = nil;
    state->started.store(false);
    state->start_failed.store(false);
  }

  delete state;
  *platform_impl = 0;
}

}  // namespace

namespace electron {

bool SystemAudioCapturer::StartPlatformCapture() {
  if (!@available(macOS 13.0, *)) {
    SetLastError("System audio capture requires macOS 13 or newer.");
    return false;
  }

  bool has_permission = CGPreflightScreenCaptureAccess();
  if (!has_permission) {
    CGRequestScreenCaptureAccess();
    has_permission = CGPreflightScreenCaptureAccess();
  }

  {
    base::AutoLock auto_lock(diagnostics_lock_);
    diagnostics_.permission_preflight = has_permission;
    diagnostics_.using_system_capture = false;
    diagnostics_.last_start_system_capture_succeeded = false;
  }

  if (!has_permission) {
    SetLastError("Screen capture permission is required for system audio capture.");
    return false;
  }

  bool succeeded = false;
  if (@available(macOS 13.0, *)) {
    succeeded = StartPlatformCaptureMac13(this, &platform_impl_);
  }

  {
    base::AutoLock auto_lock(diagnostics_lock_);
    diagnostics_.using_system_capture = succeeded;
    diagnostics_.last_start_system_capture_succeeded = succeeded;
    if (succeeded)
      diagnostics_.last_error.clear();
  }

  return succeeded;
}

void SystemAudioCapturer::StopPlatformCapture() {
  if (@available(macOS 13.0, *)) {
    StopPlatformCaptureMac13(&platform_impl_);
  } else {
    platform_impl_ = 0;
  }

  base::AutoLock auto_lock(diagnostics_lock_);
  diagnostics_.using_system_capture = false;
}

}  // namespace electron
