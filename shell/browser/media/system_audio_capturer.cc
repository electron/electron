// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/media/system_audio_capturer.h"

#include <utility>

#include "build/build_config.h"
#include "base/functional/bind.h"
#include "base/time/time.h"

namespace electron {

SystemAudioCapturer::CaptureDiagnostics::CaptureDiagnostics() = default;
SystemAudioCapturer::CaptureDiagnostics::CaptureDiagnostics(
  const CaptureDiagnostics&) = default;
SystemAudioCapturer::CaptureDiagnostics&
SystemAudioCapturer::CaptureDiagnostics::operator=(
  const CaptureDiagnostics&) = default;
SystemAudioCapturer::CaptureDiagnostics::~CaptureDiagnostics() = default;

SystemAudioCapturer::SystemAudioCapturer() = default;

SystemAudioCapturer::~SystemAudioCapturer() {
  Stop();
}

bool SystemAudioCapturer::Start(FrameCallback frame_callback,
                                ErrorCallback error_callback) {
  if (running_.exchange(true))
    return true;

  {
    base::AutoLock auto_lock(callback_lock_);
    callback_task_runner_ = base::SequencedTaskRunner::GetCurrentDefault();
    frame_callback_ = std::move(frame_callback);
    error_callback_ = std::move(error_callback);
  }
  sequence_.store(0);

  {
    base::AutoLock auto_lock(diagnostics_lock_);
    diagnostics_.system_frame_callbacks = 0;
    diagnostics_.fallback_frame_callbacks = 0;
    diagnostics_.queued_frame_depth = 0;
    diagnostics_.queue_max_depth = 0;
    diagnostics_.queue_dropped_frames = 0;
    diagnostics_.queue_underrun_frames = 0;
    diagnostics_.last_error.clear();
  }

  if (!StartPlatformCapture()) {
    running_.store(false);
    return false;
  }

  return true;
}

void SystemAudioCapturer::Stop() {
  if (!running_.exchange(false))
    return;

  StopPlatformCapture();

  {
    base::AutoLock auto_lock(callback_lock_);
    frame_callback_.Reset();
    error_callback_.Reset();
    callback_task_runner_.reset();
  }
}

SystemAudioCapturer::CaptureDiagnostics
SystemAudioCapturer::GetCaptureDiagnostics() const {
  base::AutoLock auto_lock(diagnostics_lock_);
  return diagnostics_;
}

void SystemAudioCapturer::SetLastError(std::string error_message) {
  ErrorCallback error_callback;
  scoped_refptr<base::SequencedTaskRunner> task_runner;

  {
    base::AutoLock callback_auto_lock(callback_lock_);
    error_callback = error_callback_;
    task_runner = callback_task_runner_;
  }

  {
    base::AutoLock auto_lock(diagnostics_lock_);
    diagnostics_.last_error = error_message;
  }

  if (!error_callback)
    return;

  if (task_runner && !task_runner->RunsTasksInCurrentSequence()) {
    task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](ErrorCallback callback, std::string message) {
              callback.Run(message);
            },
            error_callback, std::move(error_message)));
    return;
  }

  error_callback.Run(error_message);
}

void SystemAudioCapturer::OnPlatformError(std::string error_message) {
  if (!running_.load())
    return;

  SetLastError(std::move(error_message));
}

void SystemAudioCapturer::OnPlatformPcmData(std::vector<float> pcm,
                                            uint32_t sample_rate,
                                            uint16_t channels,
                                            double timestamp_ms,
                                            bool from_system_capture) {
  if (!running_.load())
    return;

  PcmFrameMeta meta;
  meta.sample_rate = sample_rate;
  meta.channels = channels;
  meta.sequence = sequence_.fetch_add(1);
  meta.timestamp_ms = timestamp_ms > 0.0
                          ? timestamp_ms
                          : base::Time::Now().since_origin().InMillisecondsF();
  meta.from_system_capture = from_system_capture;

  {
    base::AutoLock auto_lock(diagnostics_lock_);
    if (from_system_capture) {
      diagnostics_.system_frame_callbacks++;
    } else {
      diagnostics_.fallback_frame_callbacks++;
    }
  }

  DispatchFrame(std::move(pcm), meta);
}

void SystemAudioCapturer::DispatchFrame(std::vector<float> pcm,
                                        PcmFrameMeta meta) {
  FrameCallback frame_callback;
  scoped_refptr<base::SequencedTaskRunner> task_runner;

  {
    base::AutoLock auto_lock(callback_lock_);
    frame_callback = frame_callback_;
    task_runner = callback_task_runner_;
  }

  if (!frame_callback)
    return;

  if (task_runner && !task_runner->RunsTasksInCurrentSequence()) {
    task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](FrameCallback callback, std::vector<float> frame,
               PcmFrameMeta frame_meta) {
              callback.Run(frame, frame_meta);
            },
            frame_callback, std::move(pcm), meta));
    return;
  }

  frame_callback.Run(pcm, meta);
}

#if !BUILDFLAG(IS_MAC)
bool SystemAudioCapturer::StartPlatformCapture() {
  SetLastError("System audio capture is only available on macOS.");
  return false;
}

void SystemAudioCapturer::StopPlatformCapture() {}
#endif

}  // namespace electron
