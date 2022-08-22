const { app, UtilityProcess } = require('electron');
const path = require('path');

app.whenReady().then(() => {
  try {
    const child = new UtilityProcess(path.join(__dirname, '..', 'empty.js'));
  } catch (err) {
    process.stdout.write(err.message);
    process.stdout.end();
  }
  setImmediate(() => {
    app.quit();
  });
});
