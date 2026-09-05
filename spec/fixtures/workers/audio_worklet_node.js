// Reports whether the Node.js environment is wired up inside this
// AudioWorklet's global scope. Used by spec/fixtures/pages/audio-worklet.html
// to verify that nodeIntegrationInWorker keeps working when Blink reuses a
// pooled worker thread for multiple AudioWorklet contexts.
class NodeIntegrationProbeProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.port.onmessage = () => {
      let info;
      try {
        // require should be a function and `node:timers` should resolve.
        const ok =
          typeof require === 'function' &&
          typeof require('node:timers').setImmediate === 'function' &&
          typeof process === 'object';
        info = ok ? 'ok' : 'missing';
      } catch (err) {
        info = `throw: ${err && err.message ? err.message : err}`;
      }
      this.port.postMessage(info);
    };
  }

  process() {
    return true;
  }
}

registerProcessor('node-integration-probe', NodeIntegrationProbeProcessor);
