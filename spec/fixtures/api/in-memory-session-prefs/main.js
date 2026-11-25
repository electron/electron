const { app, session } = require('electron');

const path = require('node:path');

const userDataPath = process.argv[2];
app.setPath('userData', userDataPath);

app.whenReady().then(() => {
  session.fromPartition('in-memory').setDownloadPath(path.join(userDataPath, 'in-memory-downloads'));
  app.quit();
});
