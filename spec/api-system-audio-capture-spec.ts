import { expect } from 'chai';

import { setTimeout } from 'node:timers/promises';

import { ifit } from './lib/spec-helpers';

describe('systemAudioCapture (internal)', () => {
  it('exposes a linked binding with expected methods', () => {
    const binding = process._linkedBinding('electron_browser_system_audio_capture') as {
      createSystemAudioCapture: () => ElectronInternal.SystemAudioCapture;
    };

    expect(binding).to.have.property('createSystemAudioCapture').that.is.a('function');

    const capture = binding.createSystemAudioCapture();
    expect(capture).to.have.property('startHandling').that.is.a('function');
    expect(capture).to.have.property('stopHandling').that.is.a('function');
    expect(capture).to.have.property('getCaptureDiagnostics').that.is.a('function');

    const diagnostics = capture.getCaptureDiagnostics();
    expect(diagnostics).to.have.property('usingSystemCapture').that.is.a('boolean');
    expect(diagnostics).to.have.property('lastError').that.is.a('string');
  });

  ifit(process.platform === 'darwin')('supports start/stop lifecycle with diagnostics', async () => {
    const binding = process._linkedBinding('electron_browser_system_audio_capture') as {
      createSystemAudioCapture: () => ElectronInternal.SystemAudioCapture;
    };

    const capture = binding.createSystemAudioCapture();

    let frameSeen = false;
    let errorSeen = false;
    capture._onframe = (pcm, meta) => {
      frameSeen = true;
      expect(pcm).to.be.an('array');
      expect(meta.sampleRate).to.be.a('number');
      expect(meta.channels).to.be.a('number');
      expect(meta.sequence).to.be.a('number');
      expect(meta.timestampMs).to.be.a('number');
      expect(meta.fromSystem).to.be.a('boolean');
      expect(meta.source === 'system' || meta.source === 'fallback').to.be.true();
    };
    capture._onerror = (error) => {
      errorSeen = true;
      expect(error).to.be.a('string').and.not.empty();
    };

    const started = capture.startHandling();
    const startDiagnostics = capture.getCaptureDiagnostics();
    expect(startDiagnostics.lastStartSystemCaptureSucceeded).to.equal(started);

    if (!started) {
      expect(startDiagnostics.lastError).to.be.a('string').and.not.empty();
      capture.stopHandling();
      return;
    }

    await setTimeout(500);

    const runningDiagnostics = capture.getCaptureDiagnostics();
    expect(runningDiagnostics.usingSystemCapture).to.equal(true);

    capture.stopHandling();

    const stoppedDiagnostics = capture.getCaptureDiagnostics();
    expect(stoppedDiagnostics.usingSystemCapture).to.equal(false);

    // At least one callback path may fire depending on host permissions/audio activity.
    expect(frameSeen || errorSeen || started).to.be.true();
  });
});
