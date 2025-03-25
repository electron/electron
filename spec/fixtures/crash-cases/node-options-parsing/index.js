const { app, utilityProcess } = require('electron');

const path = require('node:path');

app.whenReady().then(() => {
  if (process.env.NODE_OPTIONS &&
      process.env.NODE_OPTIONS.trim() === '--allow-addons --allow-addons') {
    const child = utilityProcess.fork(path.join(__dirname, 'options.js'), [], {
      stdio: 'inherit',
      env: {
        NODE_OPTIONS: `--allow-addons --require ${path.join(__dirname, 'preload.js')} --enable-fips --allow-addons --enable-fips`,
        ELECTRON_ENABLE_STACK_DUMPING: 'true'
      }
    });

    child.once('exit', (code) => code ? app.exit(1) : app.quit());
  } else {
    app.exit(1);
  }
});
