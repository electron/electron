---
title: Deep Linking in Electron
description: A comprehensive guide to handling deep links in Electron applications across all platforms
slug: deep-linking
hide_title: true
---

# Deep Linking in Electron

## Overview

Deep linking allows your Electron application to be launched and handle specific URLs or protocol schemes (e.g., `myapp://`). This guide explains how to implement deep linking across different platforms and scenarios.

## Platform-Specific Behaviors

### Windows and Linux

On Windows and Linux, when a deep link is clicked:

1. If the app is not running (cold start):
   - The operating system launches the app with the URL as a command line argument
   - You need to check `process.argv` to extract the URL
2. If the app is already running:
   - The `second-instance` event is triggered with the URL in the command line arguments

### macOS

On macOS:

1. Both cold start and running scenarios are handled through the `open-url` event
2. The event must be registered as early as possible in your app's lifecycle

## Implementation Guide

### 1. Register Your Protocol

First, register your app as the handler for your custom protocol:

```javascript
const { app } = require("electron");

// Handle development vs production cases
if (process.defaultApp) {
  if (process.argv.length >= 2) {
    app.setAsDefaultProtocolClient("myapp", process.execPath, [
      path.resolve(process.argv[1]),
    ]);
  }
} else {
  app.setAsDefaultProtocolClient("myapp");
}
```

### 2. Handle Cold Start (Windows and Linux)

For Windows and Linux, check command line arguments when the app starts:

```javascript
// In your main process
function handleDeepLink(url) {
  // Handle the URL here
  console.log("Received deep link:", url);
}

// Check for deep link in cold start
const gotTheLock = app.requestSingleInstanceLock();

if (!gotTheLock) {
  app.quit();
} else {
  app.on("ready", () => {
    // Check if we have a deep link URL in process.argv
    const deepLinkUrl = process.argv.find((arg) => arg.startsWith("myapp://"));
    if (deepLinkUrl) {
      handleDeepLink(deepLinkUrl);
    }
  });

  // Handle deep links when app is already running
  app.on("second-instance", (event, commandLine) => {
    const deepLinkUrl = commandLine.find((arg) => arg.startsWith("myapp://"));
    if (deepLinkUrl) {
      handleDeepLink(deepLinkUrl);
    }
  });
}
```

### 3. Handle Deep Links on macOS

For macOS, use the `open-url` event:

```javascript
// Must be registered before app is ready
app.on("will-finish-launching", () => {
  app.on("open-url", (event, url) => {
    event.preventDefault();
    handleDeepLink(url);
  });
});
```

## Best Practices

1. **Early Registration**: Register protocol handlers and event listeners as early as possible in your app's lifecycle.

2. **Single Instance**: Use `app.requestSingleInstanceLock()` to ensure only one instance of your app runs at a time.

3. **URL Validation**: Always validate deep link URLs before processing them:

   ```javascript
   function isValidDeepLink(url) {
     try {
       const parsedUrl = new URL(url);
       return parsedUrl.protocol === "myapp:";
     } catch {
       return false;
     }
   }
   ```

4. **Error Handling**: Implement proper error handling for malformed URLs:
   ```javascript
   function handleDeepLink(url) {
     try {
       if (!isValidDeepLink(url)) {
         console.error("Invalid deep link URL:", url);
         return;
       }
       // Process the URL
     } catch (error) {
       console.error("Error handling deep link:", error);
     }
   }
   ```

## Testing Deep Links

To test deep links:

1. Register your protocol using `setAsDefaultProtocolClient`
2. Create a test HTML file with your protocol link:
   ```html
   <a href="myapp://test">Open in My App</a>
   ```
3. Test both cold start and running scenarios
4. Test with malformed URLs to ensure proper error handling

## Common Issues and Solutions

1. **Development vs Production**: Note the different syntax needed for `setAsDefaultProtocolClient` in development mode.

2. **Windows Registry**: On Windows, protocol registration requires admin privileges or user consent.

3. **Linux Desktop Entry**: On Linux, ensure your `.desktop` file includes the protocol handler:
   ```desktop
   [Desktop Entry]
   ...
   MimeType=x-scheme-handler/myapp;
   ```

## API References

- [`app.setAsDefaultProtocolClient`](../api/app.md#appsetasdefaultprotocolclientprotocol-path-args)
- [`app.removeAsDefaultProtocolClient`](../api/app.md#appremoveasdefaultprotocolclientprotocol-path-args)
- [`app.isDefaultProtocolClient`](../api/app.md#appisdefaultprotocolclientprotocol-path-args)
