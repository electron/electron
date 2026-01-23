const { dialog, session } = require('electron');
const path = require('path');
const url = require('url');

// Use original-fs for reading from ASAR archives in Electron
const fs = process.type === 'browser' ? require('original-fs') : require('fs');

/**
 * ASAR PDF Download Handler
 * 
 * Intercepts will-download events for PDF files originating from ASAR archives,
 * reads the content directly from the ASAR, shows a save dialog, and writes
 * the file manually to bypass Chromium's download process.
 */

/**
 * Parse a file URL to determine if it originates from an ASAR archive
 */
function parseAsarUrl(downloadUrl) {
  try {
    let filePath;
    
    // Handle file:// URLs
    if (downloadUrl.startsWith('file://')) {
      const parsedUrl = new url.URL(downloadUrl);
      filePath = parsedUrl.pathname;
    }
    // Handle custom protocols like app://
    else if (downloadUrl.includes('://')) {
      const parsedUrl = new url.URL(downloadUrl);
      filePath = parsedUrl.pathname;
      
      // For custom protocols, we might need to handle the path differently
      // Some apps use app://./path/to/file.asar/document.pdf
      if (filePath.startsWith('./')) {
        filePath = filePath.substring(2);
      }
    }
    // Handle direct file paths
    else {
      filePath = downloadUrl;
    }
    
    // Check if path contains .asar (case insensitive)
    const asarMatch = filePath.match(/^(.+\.asar)([/\\].*)?$/i);
    if (asarMatch) {
      return {
        isAsar: true,
        asarPath: asarMatch[1],
        filePath: asarMatch[2] ? asarMatch[2].replace(/^[/\\]/, '') : ''
      };
    }
    
    return { isAsar: false };
  } catch (error) {
    console.error('Error parsing ASAR URL:', error);
    return { isAsar: false };
  }
}

/**
 * Check if a file is a PDF based on its extension or MIME type
 */
function isPdfFile(filename, mimeType) {
  const extension = path.extname(filename).toLowerCase();
  return extension === '.pdf' || mimeType === 'application/pdf';
}

/**
 * Read file content from ASAR archive
 */
function readFromAsar(asarPath, filePath) {
  try {
    // Construct the full path within the ASAR
    const fullAsarPath = path.join(asarPath, filePath);
    
    // Use original-fs to read from ASAR (bypasses Electron's fs wrapper)
    return fs.readFileSync(fullAsarPath);
  } catch (error) {
    // Provide more specific error messages
    if (error.code === 'ENOENT') {
      throw new Error(`PDF file not found in ASAR archive: ${path.join(asarPath, filePath)}`);
    } else if (error.code === 'EACCES') {
      throw new Error(`Permission denied reading from ASAR archive: ${error.message}`);
    } else if (error.code === 'EISDIR') {
      throw new Error(`Path is a directory, not a file: ${path.join(asarPath, filePath)}`);
    } else {
      throw new Error(`Failed to read file from ASAR: ${error.message}`);
    }
  }
}

/**
 * Show save dialog and write PDF content to selected location
 */
async function saveAsarPdf(webContents, pdfContent, defaultFilename) {
  try {
    const result = await dialog.showSaveDialog(webContents, {
      title: 'Save PDF',
      defaultPath: defaultFilename,
      filters: [
        { name: 'PDF Files', extensions: ['pdf'] },
        { name: 'All Files', extensions: ['*'] }
      ],
      properties: ['createDirectory']
    });

    if (result.canceled || !result.filePath) {
      return false;
    }

    // Write the PDF content to the selected location
    fs.writeFileSync(result.filePath, pdfContent);
    return true;
  } catch (error) {
    console.error('Error saving PDF from ASAR:', error);
    return false;
  }
}

/**
 * Handle will-download event for ASAR-based PDFs
 */
async function handleAsarPdfDownload(event, item, webContents) {
  const downloadUrl = item.getURL();
  const filename = item.getFilename();
  const mimeType = item.getMimeType();

  // Check if this is a PDF from an ASAR archive
  const asarInfo = parseAsarUrl(downloadUrl);
  
  if (!asarInfo.isAsar || !isPdfFile(filename, mimeType)) {
    // Not an ASAR PDF, let default download process handle it
    return;
  }

  console.log(`Intercepting ASAR PDF download: ${downloadUrl}`);

  try {
    // Prevent the default download process
    event.preventDefault();

    // Read PDF content from ASAR
    const pdfContent = readFromAsar(asarInfo.asarPath, asarInfo.filePath || filename);

    // Show save dialog and write file
    const saved = await saveAsarPdf(webContents, pdfContent, filename);

    if (saved) {
      console.log(`Successfully saved PDF from ASAR: ${filename}`);
    } else {
      console.log(`User canceled save dialog for PDF: ${filename}`);
    }

    // Cancel the download item to clean up
    item.cancel();

  } catch (error) {
    console.error('Error handling ASAR PDF download:', error);
    
    // Show error dialog to user
    dialog.showErrorBox(
      'Download Error',
      `Failed to download PDF from archive: ${error.message}`
    );
    
    // Cancel the item since we can't recover
    item.cancel();
  }
}

/**
 * Initialize the ASAR PDF download handler for a session
 */
function initializeAsarPdfDownloadHandler(targetSession) {
  const sessionToUse = targetSession || session.defaultSession;

  // Remove any existing listeners to avoid duplicates
  sessionToUse.removeAllListeners('will-download');

  // Add our custom handler
  sessionToUse.on('will-download', handleAsarPdfDownload);

  console.log('ASAR PDF download handler initialized');
}

/**
 * Remove the ASAR PDF download handler from a session
 */
function removeAsarPdfDownloadHandler(targetSession) {
  const sessionToUse = targetSession || session.defaultSession;
  sessionToUse.removeAllListeners('will-download');
  console.log('ASAR PDF download handler removed');
}

// Export functions
module.exports = {
  initializeAsarPdfDownloadHandler,
  removeAsarPdfDownloadHandler,
  handleAsarPdfDownload,
  parseAsarUrl,
  isPdfFile,
  readFromAsar,
  saveAsarPdf
};