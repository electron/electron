(function () {
  const { setImmediate } = require('timers');
  const { ipcRenderer } = require('electron');
  window.ipcRenderer = ipcRenderer;
  window.setImmediate = setImmediate;
  window.require = require;

  function invoke (code) {
    try {
      return code();
    } catch {
      return null;
    }
  }

  process.once('loaded', () => {
    ipcRenderer.send('process-loaded');
  });

  if (location.protocol === 'file:') {
    window.test = 'preload';
    window.process = process;
    if (process.env.sandboxmain) {
      window.test = {
        osSandbox: !process.argv.includes('--no-sandbox'),
        hasCrash: typeof process.crash === 'function',
        hasHang: typeof process.hang === 'function',
        creationTime: invoke(() => process.getCreationTime()),
        heapStatistics: invoke(() => process.getHeapStatistics()),
        blinkMemoryInfo: invoke(() => process.getBlinkMemoryInfo()),
        processMemoryInfo: invoke(() => process.getProcessMemoryInfo() ? {} : null),
        systemMemoryInfo: invoke(() => process.getSystemMemoryInfo()),
        systemVersion: invoke(() => process.getSystemVersion()),
        cpuUsage: invoke(() => process.getCPUUsage()),
        ioCounters: invoke(() => process.getIOCounters()),
        uptime: invoke(() => process.uptime()),
        env: process.env,
        execPath: process.execPath,
        pid: process.pid,
        arch: process.arch,
        platform: process.platform,
        sandboxed: process.sandboxed,
        contextIsolated: process.contextIsolated,
        type: process.type,
        version: process.version,
        versions: process.versions,
        contextId: process.contextId
      };
    }
  } else if (location.href !== 'about:blank') {
    addEventListener('DOMContentLoaded', () => {
      ipcRenderer.on('touch-the-opener', () => {
        let errorMessage = null;
        try {
          const openerDoc = opener.document; // eslint-disable-line no-unused-vars
        } catch (error) {
          errorMessage = error.message;
        }
        ipcRenderer.send('answer', errorMessage);
      });
      ipcRenderer.send('child-loaded', window.opener == null, document.body.innerHTML, location.href);
    });
  }
})();
