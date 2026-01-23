# ASAR PDF Download Handler Implementation

## Overview

This implementation provides a complete solution for handling PDF downloads from ASAR archives in Electron applications. The solution intercepts `will-download` events for PDF files originating from ASAR paths, reads the content directly from the ASAR archive, shows a save dialog, and writes the file manually.

## Implementation Files

### Core Handler
- **`lib/browser/asar-pdf-download-handler.ts`** - Main implementation
  - `initializeAsarPdfDownloadHandler()` - Initialize handler for a session
  - `removeAsarPdfDownloadHandler()` - Remove handler from session
  - `handleAsarPdfDownload()` - Core event handler function
  - `parseAsarUrl()` - Parse URLs to detect ASAR paths
  - `isPdfFile()` - Identify PDF files by extension/MIME type
  - `readFromAsar()` - Read file content from ASAR archive
  - `saveAsarPdf()` - Show save dialog and write file

### Integration Helper
- **`lib/browser/main-process-integration.ts`** - Main process integration example
  - `initializeMainProcess()` - App-wide initialization pattern

### Documentation
- **`lib/browser/README.md`** - Comprehensive documentation
- **`ASAR_PDF_DOWNLOAD_IMPLEMENTATION.md`** - This implementation summary

### Testing
- **`spec/asar-pdf-download-handler-spec.ts`** - Comprehensive test suite
- **`spec/pdf-asar-download-spec.ts`** - Integration tests with existing PDF tests

### Examples
- **`examples/asar-pdf-download-example.ts`** - Usage examples and patterns

## Key Features

### 1. Automatic ASAR Detection
```typescript
function parseAsarUrl(downloadUrl: string): AsarPathInfo {
  // Detects URLs like:
  // - file:///path/to/app.asar/documents/manual.pdf
  // - /path/to/app.asar/documents/manual.pdf
  // - C:\app\resources\app.asar\docs\guide.pdf
}
```

### 2. PDF File Identification
```typescript
function isPdfFile(filename: string, mimeType?: string): boolean {
  const extension = path.extname(filename).toLowerCase();
  return extension === '.pdf' || mimeType === 'application/pdf';
}
```

### 3. Direct ASAR File Reading
```typescript
function readFromAsar(asarPath: string, filePath: string): Buffer {
  const fullAsarPath = path.join(asarPath, filePath);
  return fs.readFileSync(fullAsarPath); // Electron's fs wrapper handles ASAR
}
```

### 4. User-Friendly Save Dialog
```typescript
async function saveAsarPdf(
  webContents: Electron.WebContents,
  pdfContent: Buffer,
  defaultFilename: string
): Promise<boolean> {
  const result = await dialog.showSaveDialog(webContents, {
    title: 'Save PDF',
    defaultPath: defaultFilename,
    filters: [{ name: 'PDF Files', extensions: ['pdf'] }]
  });
  
  if (!result.canceled && result.filePath) {
    fs.writeFileSync(result.filePath, pdfContent);
    return true;
  }
  return false;
}
```

### 5. Event Interception and Cleanup
```typescript
async function handleAsarPdfDownload(
  event: Electron.Event,
  item: Electron.DownloadItem,
  webContents: Electron.WebContents
): Promise<void> {
  // 1. Check if this is an ASAR PDF
  const asarInfo = parseAsarUrl(item.getURL());
  if (!asarInfo.isAsar || !isPdfFile(item.getFilename(), item.getMimeType())) {
    return; // Let default download handle it
  }

  // 2. Prevent default download
  event.preventDefault();

  // 3. Read from ASAR and save
  const pdfContent = readFromAsar(asarInfo.asarPath!, asarInfo.filePath!);
  await saveAsarPdf(webContents, pdfContent, item.getFilename());

  // 4. Cancel the download item
  item.cancel();
}
```

## Usage Patterns

### Basic Integration
```typescript
import { app } from 'electron/main';
import { initializeAsarPdfDownloadHandler } from './lib/browser/asar-pdf-download-handler';

app.whenReady().then(() => {
  initializeAsarPdfDownloadHandler(); // For default session
});
```

### Per-Window Integration
```typescript
const mainWindow = new BrowserWindow({ /* options */ });
initializeAsarPdfDownloadHandler(mainWindow.webContents.session);
```

### Custom Session Integration
```typescript
const customSession = session.fromPartition('custom-partition');
initializeAsarPdfDownloadHandler(customSession);
```

## HTML Integration

Create HTML pages with links to ASAR PDFs:

```html
<!DOCTYPE html>
<html>
<body>
  <h1>Document Library</h1>
  <ul>
    <!-- These will be handled by our ASAR handler -->
    <li><a href="file:///app/resources/app.asar/docs/manual.pdf" download="manual.pdf">User Manual</a></li>
    <li><a href="file:///app/resources/app.asar/docs/api.pdf" download="api.pdf">API Reference</a></li>
  </ul>
  
  <!-- Regular downloads work normally -->
  <ul>
    <li><a href="https://example.com/external.pdf" download="external.pdf">External PDF</a></li>
  </ul>
</body>
</html>
```

## Error Handling

The implementation includes comprehensive error handling:

1. **File Not Found**: Shows error dialog if PDF doesn't exist in ASAR
2. **Read Errors**: Handles ASAR read failures gracefully
3. **Write Errors**: Manages file system permission issues
4. **User Cancellation**: Properly handles canceled save dialogs
5. **Fallback**: Non-ASAR downloads continue working normally

## Testing Strategy

### Unit Tests
- URL parsing validation
- File type detection
- ASAR content reading
- Save dialog interaction
- Error condition handling

### Integration Tests
- Session event handling
- End-to-end download scenarios
- Compatibility with existing PDF tests
- Cross-platform path handling

### Test Execution
```bash
npm test -- --grep "ASAR PDF Download Handler"
npm test -- --grep "PDF viewer ASAR download integration"
```

## Performance Considerations

1. **Synchronous Operations**: Uses sync file operations for simplicity
2. **Memory Usage**: Loads entire PDF into memory (suitable for typical documents)
3. **ASAR Access**: Leverages Electron's optimized ASAR file system wrapper
4. **Event Handling**: Minimal overhead for non-ASAR downloads

## Security Features

1. **Path Validation**: Only processes files from ASAR archives
2. **File Type Checking**: Validates PDF files before processing
3. **Dialog Security**: Uses Electron's native save dialog
4. **Error Boundaries**: Prevents crashes from malformed requests

## Compatibility

- **Electron Versions**: Compatible with modern Electron versions
- **Operating Systems**: Windows, macOS, Linux
- **ASAR Format**: Works with standard Electron ASAR archives
- **PDF Viewer**: Compatible with Electron's built-in PDF viewer

## Limitations

1. **PDF Only**: Currently handles PDF files only (extensible)
2. **Sync Operations**: Uses synchronous file operations
3. **Memory Bound**: Loads entire file into memory
4. **ASAR Specific**: Only works with ASAR archive format

## Future Enhancements

1. **Async Operations**: Convert to async file operations
2. **Progress Indicators**: Add progress bars for large files
3. **Multiple File Types**: Extend to handle other document types
4. **Streaming**: Implement streaming for very large files
5. **Caching**: Add file content caching for repeated downloads

## Troubleshooting

### Common Issues

1. **Handler Not Working**
   - Verify initialization: `initializeAsarPdfDownloadHandler()`
   - Check URL contains `.asar`
   - Confirm file has `.pdf` extension

2. **File Not Found**
   - Verify ASAR archive exists
   - Check PDF is included in ASAR during build
   - Test ASAR manually: `asar list app.asar`

3. **Save Dialog Issues**
   - Check console for errors
   - Verify WebContents is valid
   - Ensure app has proper permissions

### Debug Logging

The handler includes console logging for debugging:
- Handler initialization
- ASAR PDF detection
- Successful saves
- Error conditions

## Integration Checklist

- [ ] Import handler in main process
- [ ] Initialize handler after app ready
- [ ] Test with ASAR PDF links
- [ ] Verify save dialog appears
- [ ] Confirm files save correctly
- [ ] Test error conditions
- [ ] Verify non-ASAR downloads still work
- [ ] Add cleanup on app quit (optional)

This implementation provides a robust, user-friendly solution for handling PDF downloads from ASAR archives while maintaining compatibility with existing download functionality.