// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_MEDIA_SYSTEM_AUDIO_CAPTURER_H_
#define ELECTRON_SHELL_BROWSER_MEDIA_SYSTEM_AUDIO_CAPTURER_H_

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/synchronization/lock.h"
#include "base/task/sequenced_task_runner.h"

namespace electron {

class SystemAudioCapturer {
 public:
  struct PcmFrameMeta {
    uint32_t sample_rate = 0;
    uint16_t channels = 0;
    uint64_t sequence = 0;
    double timestamp_ms = 0.0;
    bool from_system_capture = false;
  };

  struct CaptureDiagnostics {
    CaptureDiagnostics();
    CaptureDiagnostics(const CaptureDiagnostics&);
    CaptureDiagnostics& operator=(const CaptureDiagnostics&);
    ~CaptureDiagnostics();

    bool permission_preflight = false;
    bool using_system_capture = false;
    bool last_start_system_capture_succeeded = false;
    uint64_t system_frame_callbacks = 0;
    uint64_t fallback_frame_callbacks = 0;
    uint64_t queued_frame_depth = 0;
    uint64_t queue_max_depth = 0;
    uint64_t queue_dropped_frames = 0;
    uint64_t queue_underrun_frames = 0;
    std::string last_error;
  };

  using FrameCallback =
      base::RepeatingCallback<void(const std::vector<float>&, const PcmFrameMeta&)>;
  using ErrorCallback = base::RepeatingCallback<void(const std::string&)>;

  SystemAudioCapturer();
  ~SystemAudioCapturer();

  bool Start(FrameCallback frame_callback, ErrorCallback error_callback);
  void Stop();

  bool is_running() const { return running_.load(); }
  CaptureDiagnostics GetCaptureDiagnostics() const;

  // Platform hook methods used by Objective-C++ output handlers.
  void OnPlatformError(std::string error_message);
  void OnPlatformPcmData(std::vector<float> pcm,
                         uint32_t sample_rate,
                         uint16_t channels,
                         double timestamp_ms,
                         bool from_system_capture);

  SystemAudioCapturer(const SystemAudioCapturer&) = delete;
  SystemAudioCapturer& operator=(const SystemAudioCapturer&) = delete;

 private:
  bool StartPlatformCapture();
  void StopPlatformCapture();
  void SetLastError(std::string error_message);
  void DispatchFrame(std::vector<float> pcm, PcmFrameMeta meta);

  std::atomic<bool> running_{false};
  std::atomic<uint64_t> sequence_{0};

  mutable base::Lock diagnostics_lock_;
  CaptureDiagnostics diagnostics_ GUARDED_BY(diagnostics_lock_);

  mutable base::Lock callback_lock_;
  scoped_refptr<base::SequencedTaskRunner> callback_task_runner_
      GUARDED_BY(callback_lock_);
  FrameCallback frame_callback_ GUARDED_BY(callback_lock_);
  ErrorCallback error_callback_ GUARDED_BY(callback_lock_);

  uintptr_t platform_impl_ = 0;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_MEDIA_SYSTEM_AUDIO_CAPTURER_H_
