const { app, BrowserWindow, screen } = require('electron');

const os = require('node:os');
const path = require('node:path');

const sharedUserData = path.join(os.tmpdir(), 'electron-window-state-test');
app.setPath('userData', sharedUserData);

app.whenReady().then(async () => {
  const displays = screen.getAllDisplays();
  const primaryDisplay = screen.getPrimaryDisplay();

  // Exit with code 1 if only one display (test will skip)
  if (displays.length < 2) {
    app.exit(1);
    return;
  }

  const secondaryDisplay = displays.find(display => display.id !== primaryDisplay.id);

  // Exit with code 1 if no secondary display found (test will skip)
  if (!secondaryDisplay) {
    app.exit(1);
    return;
  }

  const workArea = secondaryDisplay.workArea;

  const maxWidth = Math.max(200, Math.floor(workArea.width * 0.8));
  const maxHeight = Math.max(150, Math.floor(workArea.height * 0.8));
  const windowWidth = Math.min(400, maxWidth);
  const windowHeight = Math.min(300, maxHeight);

  const w = new BrowserWindow({
    width: windowWidth,
    height: windowHeight,
    windowStateRestoreOptions: {
      stateId: 'test-work-area-secondary'
    }
  });

  // Center the window on the secondary display to prevent overflow
  const centerX = workArea.x + Math.floor((workArea.width - windowWidth) / 2);
  const centerY = workArea.y + Math.floor((workArea.height - windowHeight) / 2);

  w.setPosition(centerX, centerY);

  w.on('close', () => {
    app.quit();
  });

  w.close();

  // Timeout of 10s to ensure app exits
  setTimeout(() => {
    app.quit();
  }, 10000);
});
