const { app, BrowserWindow } = require('electron');

const scenario = process.argv.find(a => a.startsWith('--scenario='))?.split('=')[1] || 'leak';

app.commandLine.appendSwitch('js-flags', '--max-old-space-size=32');

app.whenReady().then(() => {
  const w = new BrowserWindow({ show: false });

  w.webContents.on('render-process-gone', () => process.exit(0));

  w.webContents.on('did-finish-load', () => {
    const scripts = {
      leak: `
        function leakMemory() {
          const arr = [];
          while (true) { arr.push(new Array(1000).fill("x".repeat(1000))); }
        }
        function triggerOom() { leakMemory(); }
        triggerOom();
      `,
      json: `
        function buildGiantObject() {
          const obj = {};
          for (let i = 0; i < 100000; i++) {
            obj['key' + i] = 'x'.repeat(1000);
          }
          return obj;
        }
        function serializeData() {
          const big = buildGiantObject();
          JSON.stringify(big);
        }
        serializeData();
      `,
      'worker-leak': `
        const blob = new Blob([\`
          function workerLeakMemory() {
            const arr = [];
            while (true) { arr.push(new Array(1000).fill("x".repeat(1000))); }
          }
          function triggerWorkerOom() { workerLeakMemory(); }
          triggerWorkerOom();
        \`], { type: 'application/javascript' });
        const worker = new Worker(URL.createObjectURL(blob));
      `
    };
    w.webContents.executeJavaScript(scripts[scenario] || scripts.leak).catch(() => {});
  });

  w.loadURL('about:blank');
});

app.on('child-process-gone', (_event, details) => {
  if (details.type === 'renderer') process.exit(0);
});

setTimeout(() => process.exit(2), 30000);
