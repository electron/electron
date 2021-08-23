const { app, BrowserWindow } = require('electron');

app.commandLine.appendSwitch('--disable-software-rasterizer');

app.whenReady().then(() => {
  const infoType = process.argv.pop();
  const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
  w.webContents.once('did-finish-load', () => {
    app.getGPUInfo(infoType).then(
      (gpuInfo) => {
        console.log('HERE COMES THE JSON: ' + JSON.stringify(gpuInfo) + ' AND THERE IT WAS');
        setImmediate(() => app.exit(0));
      },
      (error) => {
        console.error(error);
        setImmediate(() => app.exit(1));
      }
    );
  });
  w.loadURL('data:text/html;<canvas></canvas>');
});
