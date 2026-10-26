// Using the LanguageModel API from a Worker while a local AI handler is
// registered must not crash the main process.
const { app, BrowserWindow, session, utilityProcess } = require('electron');

const path = require('node:path');

app.commandLine.appendSwitch('enable-blink-features', 'AIPromptAPIForWorkers');

app.whenReady().then(async () => {
  const handler = utilityProcess.fork(path.join(__dirname, 'handler.js'));
  session.defaultSession.registerLocalAIHandler(handler);

  const w = new BrowserWindow({ show: false });
  await w.loadFile(path.join(__dirname, 'index.html'));
  const result = await w.webContents.executeJavaScript(`
    new Promise((resolve) => {
      const worker = new Worker('worker.js');
      worker.onmessage = (e) => resolve(e.data);
      worker.onerror = (e) => resolve('error: ' + e.message);
    })
  `);
  if (typeof result !== 'string') process.exitCode = 1;
  handler.kill();
  app.quit();
});
