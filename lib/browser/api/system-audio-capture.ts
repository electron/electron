const { createSystemAudioCapture } = process._linkedBinding(
  'electron_browser_system_audio_capture'
);

export function createCapture() {
  return createSystemAudioCapture();
}

export function getCaptureDiagnostics() {
  const capture = createSystemAudioCapture();
  return capture.getCaptureDiagnostics();
}

export function startCapture(
  onFrame: ElectronInternal.SystemAudioFrameHandler,
  onError?: (error: string) => void
) {
  const capture = createSystemAudioCapture();
  capture._onframe = onFrame;
  if (onError) capture._onerror = onError;

  if (!capture.startHandling()) {
    const diagnostics = capture.getCaptureDiagnostics();
    throw new Error(diagnostics.lastError || 'Failed to start system audio capture');
  }

  return {
    stop: () => capture.stopHandling(),
    getDiagnostics: () => capture.getCaptureDiagnostics()
  };
}
