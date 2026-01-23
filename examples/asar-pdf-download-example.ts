/**
 * ASAR PDF Download Handler - Usage Example
 * 
 * This example demonstrates how to integrate the ASAR PDF download handler
 * into your Electron application to handle PDF downloads from ASAR archives.
 */

import { app, BrowserWindow } from 'electron/main';
import { initializeAsarPdfDownloadHandler } from '../lib/browser/asar-pdf-download-handler';
import * as path from 'node:path';

// Example 1: Basic Integration
function createMainWindow(): BrowserWindow {
  const mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js')
    }
  });

  // Initialize ASAR PDF download handler for this window's session
  initializeAsarPdfDownloadHandler(mainWindow.webContents.session);

  return mainWindow;
}

// Example 2: Application-wide Integration
app.whenReady().then(() => {
  // Initialize for the default session (affects all windows using default session)
  initializeAsarPdfDownloadHandler();

  const mainWindow = createMainWindow();
  
  // Load your app's main page
  mainWindow.loadFile('index.html');

  // Example: Load a page that contains links to PDFs in ASAR archives
  // mainWindow.loadURL('file:///path/to/your/app.asar/pages/documents.html');
});

// Example 3: Custom Session Integration
app.whenReady().then(() => {
  const customSession = require('electron').session.fromPartition('custom-partition');
  
  // Initialize handler for custom session
  initializeAsarPdfDownloadHandler(customSession);

  const windowWithCustomSession = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      session: customSession,
      nodeIntegration: false,
      contextIsolation: true
    }
  });

  windowWithCustomSession.loadFile('custom-page.html');
});

// Example 4: Conditional Handler Setup
app.whenReady().then(() => {
  const isDevelopment = process.env.NODE_ENV === 'development';
  
  if (isDevelopment) {
    // In development, you might want to handle ASAR PDFs differently
    console.log('Development mode: ASAR PDF handler enabled');
    initializeAsarPdfDownloadHandler();
  } else {
    // In production, always enable the handler
    initializeAsarPdfDownloadHandler();
  }
});

// Example 5: HTML Page with ASAR PDF Links
/*
Create an HTML file (e.g., documents.html) with links to PDFs in your ASAR:

<!DOCTYPE html>
<html>
<head>
  <title>Document Library</title>
</head>
<body>
  <h1>Available Documents</h1>
  
  <!-- These links will be handled by our ASAR PDF download handler -->
  <ul>
    <li><a href="file:///app/resources/app.asar/docs/manual.pdf" download="user-manual.pdf">User Manual</a></li>
    <li><a href="file:///app/resources/app.asar/docs/api-reference.pdf" download="api-reference.pdf">API Reference</a></li>
    <li><a href="file:///app/resources/app.asar/docs/tutorial.pdf" download="tutorial.pdf">Tutorial</a></li>
  </ul>

  <!-- Regular links (not from ASAR) will use default download behavior -->
  <ul>
    <li><a href="https://example.com/external-doc.pdf" download="external-doc.pdf">External Document</a></li>
  </ul>
</body>
</html>
*/

// Example 6: Preload Script for Renderer Process
/*
Create a preload script (preload.js) to safely expose functionality to renderer:

const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
  // You can add custom methods here if needed
  downloadPdf: (asarPath, filename) => {
    // Trigger download programmatically
    const link = document.createElement('a');
    link.href = asarPath;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  }
});
*/

// Example 7: Error Handling and Logging
app.whenReady().then(() => {
  try {
    initializeAsarPdfDownloadHandler();
    console.log('ASAR PDF download handler initialized successfully');
  } catch (error) {
    console.error('Failed to initialize ASAR PDF download handler:', error);
    
    // Fallback: continue without the handler
    // Regular downloads will still work normally
  }
});

// Example 8: Cleanup on App Quit
app.on('before-quit', () => {
  // The handler automatically cleans up, but you can be explicit
  console.log('Application shutting down...');
});

export { createMainWindow };