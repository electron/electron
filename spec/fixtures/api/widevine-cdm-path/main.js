const path = require('node:path');

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
    await window.loadFile(path.join(__dirname, '..', '..', 'pages', 'blank.html'));
    const result = await window.webContents.executeJavaScript(`
      (async () => ({
        isSecureContext: window.isSecureContext,
        widevineAvailable: await navigator.requestMediaKeySystemAccess(
          'com.widevine.alpha',
          ${JSON.stringify(config)}
        ).then(() => true, () => false)
      }))()
    `);

    process.stdout.write(
      `WIDEVINE_CDM_PATH_RESULT=${JSON.stringify({
        isPackaged: app.isPackaged,
        ...result
      })}\n`
    );
    app.quit();
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });
