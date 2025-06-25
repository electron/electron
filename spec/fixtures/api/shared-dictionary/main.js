const { app, BrowserWindow, session } = require('electron');

const path = require('node:path');

app.setPath('userData', path.join(__dirname, 'user-data-dir'));

// Grab the command to run from process.argv
const command = process.argv[2];
app.whenReady().then(async () => {
  const bw = new BrowserWindow({ show: true });
  await bw.loadURL('https://compression-dictionary-transport-threejs-demo.glitch.me/demo.html?r=151');

  // Wait a second for glitch to load, it sometimes takes a while
  // if the glitch app is booting up (did-finish-load will fire too soon)
  await new Promise(resolve => setTimeout(resolve, 1000));

  try {
    let result;
    const isolationKey = {
      frameOrigin: 'https://compression-dictionary-transport-threejs-demo.glitch.me',
      topFrameSite: 'https://compression-dictionary-transport-threejs-demo.glitch.me'
    };

    if (command === 'getSharedDictionaryInfo') {
      result = await session.defaultSession.getSharedDictionaryInfo(isolationKey);
    } else if (command === 'getSharedDictionaryUsageInfo') {
      result = await session.defaultSession.getSharedDictionaryUsageInfo();
    } else if (command === 'clearSharedDictionaryCache') {
      await session.defaultSession.clearSharedDictionaryCache();
      result = await session.defaultSession.getSharedDictionaryUsageInfo();
    } else if (command === 'clearSharedDictionaryCacheForIsolationKey') {
      await session.defaultSession.clearSharedDictionaryCacheForIsolationKey(isolationKey);
      result = await session.defaultSession.getSharedDictionaryUsageInfo();
    }

    console.log(JSON.stringify(result));
  } catch (e) {
    console.log('error', e);
  } finally {
    app.quit();
  }
});
