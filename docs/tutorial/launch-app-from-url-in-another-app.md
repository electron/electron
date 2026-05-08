## Handling Deep Links on Cold Start

When a user clicks a deep link and your application is **not yet running**, the behavior differs between platforms:

### macOS

On macOS, the `open-url` event is fired when the app starts. However, you must register the event listener **synchronously** at the very beginning of your main process, before any asynchronous operations:

```javascript
const { app, BrowserWindow } = require('electron');

// Register listener SYNCHRONOUSLY - this must happen immediately
app.on('open-url', (event, url) => {
  event.preventDefault();
  console.log('Deep link received:', url);
  // Handle the deep link URL
  createWindow(url);
});

// Only after the listener is registered can you do async work
app.whenReady().then(() => {
  // ... rest of your code
});
```

**Important:** If you register the `open-url` listener inside an async function or after `app.whenReady()`, you may miss the event on cold start.

### Windows and Linux

On Windows and Linux, when your app is not running and a user clicks a deep link, the URL is passed as a command-line argument. You must read `process.argv` at the start of your application, **before** `app.whenReady()`:

```javascript
const { app, BrowserWindow, dialog } = require('electron');

// Handle cold start BEFORE app.whenReady()
let deepLinkUrl = null;

function handleDeepLinkOnColdStart() {
  // Look for your custom protocol in command-line arguments
  const url = process.argv.find(arg => arg.startsWith('myapp://'));
  if (url) {
    deepLinkUrl = url;
    console.log('Deep link received on cold start:', url);
  }
}

handleDeepLinkOnColdStart();

// Later, when creating your window, use the URL
app.whenReady().then(() => {
  createWindow();
  if (deepLinkUrl) {
    mainWindow.webContents.send('deep-link', deepLinkUrl);
  }
});
```

## Handling Deep Links When App is Already Running

### macOS

Use the `open-url` event (same as cold start):

```javascript
app.on('open-url', (event, url) => {
  event.preventDefault();
  console.log('Deep link received:', url);
  // Handle the URL
  if (mainWindow) {
    mainWindow.webContents.send('deep-link', url);
  }
});
```

### Windows and Linux

When the app is already running, the `second-instance` event is emitted:

```javascript
app.on('second-instance', (event, argv) => {
  // argv contains command-line arguments from the new instance
  const url = argv.find(arg => arg.startsWith('myapp://'));
  if (url) {
    console.log('Deep link received (app running):', url);
    if (mainWindow) {
      mainWindow.webContents.send('deep-link', url);
    }
  }
});
```

## Setting Up Protocol Handler

Use `app.setAsDefaultProtocolClient()` to register your custom protocol. The method has two different call signatures depending on your application state:

### Development (with custom app launcher)

When running in development mode, use the extended form:

```javascript
if (process.defaultApp) {
  if (process.argv.length >= 2) {
    app.setAsDefaultProtocolClient('myapp', process.execPath, [
      path.resolve(process.argv[1])
    ]);
  }
} else {
  app.setAsDefaultProtocolClient('myapp');
}
```

**Why?** The `process.defaultApp` check detects if you're running the unpacked Electron app in development. In this case, you need to explicitly tell the operating system:
- Which executable to run (`process.execPath`)
- What arguments to pass (`[path.resolve(process.argv[1])]`)

This ensures the OS knows how to launch your app when a deep link is clicked. In production (packaged app), `process.defaultApp` is `false`, so the simpler form is used.

### Production (packaged app)

When your app is packaged, simply use:

```javascript
app.setAsDefaultProtocolClient('myapp');
```

The OS will automatically know how to launch your packaged application.

## Complete Example

Here's a fully functional example that works across all platforms:

```javascript
const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');

let mainWindow;
let deepLinkUrl = null;

// Handle deep link URL on cold start (Windows & Linux)
function handleDeepLinkOnColdStart() {
  const url = process.argv.find(arg => arg.startsWith('myapp://'));
  if (url) {
    deepLinkUrl = url;
  }
}

handleDeepLinkOnColdStart();

// Register protocol handler
if (process.defaultApp) {
  if (process.argv.length >= 2) {
    app.setAsDefaultProtocolClient('myapp', process.execPath, [
      path.resolve(process.argv[1])
    ]);
  }
} else {
  app.setAsDefaultProtocolClient('myapp');
}

// Handle deep links on macOS (SYNCHRONOUSLY)
app.on('open-url', (event, url) => {
  event.preventDefault();
  deepLinkUrl = url;
  if (mainWindow) {
    mainWindow.webContents.send('deep-link', url);
  }
});

// Handle deep links when app is already running (Windows & Linux)
app.on('second-instance', (event, argv) => {
  const url = argv.find(arg => arg.startsWith('myapp://'));
  if (url) {
    deepLinkUrl = url;
    if (mainWindow) {
      mainWindow.webContents.send('deep-link', url);
    }
  }
});

function createWindow() {
  mainWindow = new BrowserWindow({
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  });

  mainWindow.loadFile('index.html');

  // Send the deep link URL if one was received
  if (deepLinkUrl) {
    mainWindow.webContents.send('deep-link', deepLinkUrl);
    deepLinkUrl = null;
  }
}

app.whenReady().then(() => {
  createWindow();

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit();
});
```

In your renderer process (`index.html` or corresponding script):

```javascript
const { ipcRenderer } = require('electron');

// Listen for deep link events
ipcRenderer.on('deep-link', (event, url) => {
  console.log('Received deep link:', url);
  // Handle the URL in your app
  // For example: navigate to a route, show a dialog, etc.
});
```

## Summary

| Scenario | Platform | Event/Method | Notes |
|----------|----------|--------------|-------|
| Cold Start | macOS | `open-url` event | Register listener synchronously at app start |
| Cold Start | Windows/Linux | `process.argv` | Read before `app.whenReady()` |
| App Running | macOS | `open-url` event | Same as cold start |
| App Running | Windows/Linux | `second-instance` event | Fired when new instance started |
| Setup | All | `setAsDefaultProtocolClient()` | Use extended form in dev, simple form in production |