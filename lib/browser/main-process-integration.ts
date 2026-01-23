import { app } from 'electron/main';
import { initializeAsarPdfDownloadHandler, removeAsarPdfDownloadHandler } from './asar-pdf-download-handler';

/**
 * Main Process Integration for ASAR PDF Download Handler
 * 
 * This file demonstrates how to integrate the ASAR PDF download handler
 * into your Electron application's main process.
 */

/**
 * Initialize the ASAR PDF download handler when the app is ready
 */
function initializeMainProcess(): void {
  app.whenReady().then(() => {
    // Initialize the ASAR PDF download handler for the default session
    initializeAsarPdfDownloadHandler();
    
    console.log('Main process initialized with ASAR PDF download handler');
  });

  // Clean up when the app is about to quit
  app.on('before-quit', () => {
    removeAsarPdfDownloadHandler();
    console.log('ASAR PDF download handler cleaned up');
  });
}

// Example usage in your main.ts or main.js file:
/*
import { initializeMainProcess } from './lib/browser/main-process-integration';

// Initialize the main process with ASAR PDF handling
initializeMainProcess();

// Or manually initialize for specific sessions:
import { initializeAsarPdfDownloadHandler } from './lib/browser/asar-pdf-download-handler';
import { session } from 'electron/main';

app.whenReady().then(() => {
  // For default session
  initializeAsarPdfDownloadHandler();
  
  // For a custom session
  const customSession = session.fromPartition('custom-partition');
  initializeAsarPdfDownloadHandler(customSession);
});
*/

export { initializeMainProcess };