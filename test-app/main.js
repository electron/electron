const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const fs = require('fs');

// Import our ASAR PDF download handler
const { initializeAsarPdfDownloadHandler, removeAsarPdfDownloadHandler } = require('../lib/browser/asar-pdf-download-handler.js');

let mainWindow;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      webSecurity: false // For testing purposes only
    }
  });

  // Initialize our ASAR PDF download handler
  console.log('Initializing ASAR PDF download handler...');
  initializeAsarPdfDownloadHandler(mainWindow.webContents.session);

  // Add logging to verify our handler is working
  mainWindow.webContents.session.on('will-download', (event, item, webContents) => {
    console.log('will-download event fired for:', item.getURL());
    console.log('Filename:', item.getFilename());
    console.log('MIME type:', item.getMimeType());
  });

  // Load the test page
  mainWindow.loadFile(path.join(__dirname, 'index.html'));

  // Open DevTools for debugging
  mainWindow.webContents.openDevTools();

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

// Create a mock ASAR file with a PDF for testing
function createMockAsarWithPdf() {
  const mockAsarDir = path.join(__dirname, 'mock.asar');
  const mockPdfPath = path.join(mockAsarDir, 'test.pdf');

  // Create directory structure
  if (!fs.existsSync(mockAsarDir)) {
    fs.mkdirSync(mockAsarDir, { recursive: true });
  }

  // Create a simple PDF file (minimal valid PDF structure)
  const pdfContent = `%PDF-1.4
1 0 obj
<<
/Type /Catalog
/Pages 2 0 R
>>
endobj

2 0 obj
<<
/Type /Pages
/Kids [3 0 R]
/Count 1
>>
endobj

3 0 obj
<<
/Type /Page
/Parent 2 0 R
/MediaBox [0 0 612 792]
/Contents 4 0 R
>>
endobj

4 0 obj
<<
/Length 44
>>
stream
BT
/F1 12 Tf
72 720 Td
(Test PDF from ASAR) Tj
ET
endstream
endobj

xref
0 5
0000000000 65535 f 
0000000009 00000 n 
0000000058 00000 n 
0000000115 00000 n 
0000000206 00000 n 
trailer
<<
/Size 5
/Root 1 0 R
>>
startxref
299
%%EOF`;

  fs.writeFileSync(mockPdfPath, pdfContent);
  console.log('Created mock PDF at:', mockPdfPath);
  return mockPdfPath;
}

// IPC handlers for testing
ipcMain.handle('get-mock-pdf-path', () => {
  return createMockAsarWithPdf();
});

ipcMain.handle('trigger-download', (event, url, filename) => {
  console.log('Triggering download for:', url);
  mainWindow.webContents.downloadURL(url);
});

app.whenReady().then(() => {
  createWindow();
  
  // Create mock ASAR with PDF
  createMockAsarWithPdf();
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});

app.on('before-quit', () => {
  console.log('Cleaning up ASAR PDF download handler...');
  if (mainWindow) {
    removeAsarPdfDownloadHandler(mainWindow.webContents.session);
  }
});