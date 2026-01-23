# ASAR PDF Download Handler

This module provides a solution for handling PDF downloads from ASAR archives in Electron applications. When a PDF is requested for download from within an ASAR archive, this handler intercepts the download, reads the file directly from the ASAR, presents a save dialog to the user, and writes the file to the selected location.

## Problem

By default, Electron's download system may not properly handle files located within ASAR archives, particularly for PDF files that are viewed in the built-in PDF viewer. This can result in:

- `will-download` events not firing correctly
- Incorrect file paths in download items
- Failed downloads from ASAR-packaged PDFs
- Inconsistent behavior between development and production builds

## Solution

The ASAR PDF Download Handler provides:

1. **Automatic Detection**: Identifies when a download URL originates from an ASAR archive
2. **Direct File Access**: Reads PDF content directly from the ASAR using Node.js fs module
3. **User-Friendly Save Dialog**: Uses Electron's native save dialog for file location selection
4. **Seamless Integration**: Works alongside normal download handling for non-ASAR files
5. **Error Handling**: Graceful fallback and user notification for errors

## Files

- `asar-pdf-download-handler.ts` - Main handler implementation
- `main-process-integration.ts` - Integration helper for main process
- `README.md` - This documentation
- `../spec/asar-pdf-download-handler-spec.ts` - Comprehensive test suite
- `../examples/asar-pdf-download-example.ts` - Usage examples

## Quick Start

### Basic Usage

```typescript
import { app } from 'electron/main';
import { initializeAsarPdfDownloadHandler } from './lib/browser/asar-pdf-download-handler';

app.whenReady().then(() => {
  // Initialize for default session (affects all windows)
  initializeAsarPdfDownloadHandler();
  
  // Create your application windows...
});
```

### Per-Window Usage

```typescript
import { BrowserWindow } from 'electron/main';
import { initializeAsarPdfDownloadHandler } from './lib/browser/asar-pdf-download-handler';

const mainWindow = new BrowserWindow({
  // window options...
});

// Initialize for this specific window's session
initializeAsarPdfDownloadHandler(mainWindow.webContents.session);
```

## API Reference

### `initializeAsarPdfDownloadHandler(session?: Electron.Session)`

Initializes the ASAR PDF download handler for the specified session.

- `session` (optional): The Electron session to attach the handler to. Defaults to `session.defaultSession`.

### `removeAsarPdfDownloadHandler(session?: Electron.Session)`

Removes the ASAR PDF download handler from the specified session.

- `session` (optional): The Electron session to remove the handler from. Defaults to `session.defaultSession`.

### `handleAsarPdfDownload(event, item, webContents)`

The core handler function (used internally by the session event listener).

- `event`: The will-download event object
- `item`: The DownloadItem object
- `webContents`: The WebContents that initiated the download

## How It Works

1. **Event Interception**: Listens for `will-download` events on the specified session
2. **URL Analysis**: Parses the download URL to determine if it originates from an ASAR archive
3. **File Type Check**: Verifies the file is a PDF (by extension or MIME type)
4. **Content Reading**: Uses Node.js `fs.readFileSync()` to read the PDF data from the ASAR
5. **Save Dialog**: Shows `dialog.showSaveDialog()` for user to choose save location
6. **File Writing**: Writes the PDF content to the selected location using `fs.writeFileSync()`
7. **Cleanup**: Calls `item.cancel()` to prevent the default Chromium download process

## Supported URL Formats

The handler recognizes these ASAR URL patterns:

- `file:///path/to/app.asar/documents/manual.pdf`
- `/path/to/app.asar/documents/manual.pdf`
- `C:\\app\\resources\\app.asar\\docs\\guide.pdf` (Windows)

## Error Handling

The handler includes comprehensive error handling:

- **File Not Found**: Shows error dialog if PDF doesn't exist in ASAR
- **Read Errors**: Handles ASAR read failures gracefully
- **Write Errors**: Manages file system write permission issues
- **User Cancellation**: Properly handles when user cancels save dialog
- **Fallback**: Non-ASAR downloads continue to work normally

## Testing

Run the test suite:

```bash
npm test -- --grep "ASAR PDF Download Handler"
```

The test suite includes:

- URL parsing validation
- File type detection
- ASAR content reading
- Save dialog interaction
- Error condition handling
- Session integration
- End-to-end scenarios

## Limitations

1. **PDF Files Only**: Currently only handles PDF files (can be extended for other types)
2. **Synchronous Operations**: Uses synchronous file operations (could be made async)
3. **Memory Usage**: Loads entire PDF into memory (suitable for typical document sizes)
4. **ASAR Format**: Only works with Electron's ASAR archive format

## Extending the Handler

To support additional file types, modify the `isPdfFile()` function:

```typescript
function isPdfFile(filename: string, mimeType?: string): boolean {
  const extension = path.extname(filename).toLowerCase();
  const pdfExtensions = ['.pdf'];
  const pdfMimeTypes = ['application/pdf'];
  
  // Add more types here:
  // const docExtensions = ['.doc', '.docx'];
  // const docMimeTypes = ['application/msword', 'application/vnd.openxmlformats-officedocument.wordprocessingml.document'];
  
  return pdfExtensions.includes(extension) || pdfMimeTypes.includes(mimeType || '');
}
```

## Security Considerations

- The handler only processes files from ASAR archives (not arbitrary file system paths)
- Uses Electron's built-in dialog for save location (respects OS security)
- Validates file types before processing
- Includes error boundaries to prevent crashes

## Performance Notes

- File reading is synchronous and blocks the main process briefly
- Memory usage scales with PDF file size
- Consider implementing progress indicators for large files
- ASAR file access is generally fast due to archive format optimization

## Troubleshooting

### Handler Not Working

1. Verify the handler is initialized: `initializeAsarPdfDownloadHandler()`
2. Check that the URL contains `.asar` in the path
3. Confirm the file has a `.pdf` extension or `application/pdf` MIME type
4. Ensure the PDF exists within the ASAR archive

### Save Dialog Not Appearing

1. Check console for error messages
2. Verify the WebContents object is valid
3. Ensure the application has proper permissions
4. Test with a simple PDF file first

### File Not Found Errors

1. Verify the ASAR archive exists at the specified path
2. Check that the PDF file is included in the ASAR during build
3. Ensure file paths use forward slashes in URLs
4. Test the ASAR archive manually with `asar list`

## Contributing

When contributing to this module:

1. Add tests for new functionality
2. Update documentation for API changes
3. Follow existing code style and patterns
4. Consider backward compatibility
5. Test on multiple platforms (Windows, macOS, Linux)