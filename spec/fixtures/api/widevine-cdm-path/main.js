const { app, BrowserWindow } = require('electron');

const config = [
  {
    initDataTypes: ['cenc'],
    videoCapabilities: [
      {
        contentType: 'video/webm; codecs="vp8"'
      }
    ],
    audioCapabilities: [
      {
        contentType: 'audio/webm; codecs="opus"'
      }
    ]
  }
];

app
  .whenReady()
  .then(async () => {
    const window = new BrowserWindow({ show: false });
    await window.loadURL('data:text/html,');
    const widevineAvailable = await window.webContents.executeJavaScript(`
    navigator.requestMediaKeySystemAccess(
      'com.widevine.alpha',
      ${JSON.stringify(config)}
    ).then(() => true, () => false)
  `);

    process.stdout.write(
      JSON.stringify({
        isPackaged: app.isPackaged,
        widevineAvailable
      })
    );
    app.quit();
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });
