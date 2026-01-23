# PDF Viewer ASAR Download Issue - Reproduction Test Case

## Issue Description

When a PDF is loaded from an ASAR archive and triggers a download event, the `will-download` event may not fire correctly or may have incorrect file paths. This affects applications that package PDFs within ASAR archives and need to handle download events.

## Analysis Summary

### Key Components Identified

1. **PDF Viewer Initialization**: 
   - Controlled by `isPDFViewerEnabled()` feature flag
   - Located in `shell/browser/electron_pdf_document_helper_client.cc`
   - Uses Chrome's PDF viewer with OOPIF (Out-of-Process iFrames) support

2. **Will-Download Event Handling**:
   - Primary implementation in `shell/browser/api/electron_api_session.cc`
   - Event fired in `OnDownloadCreated()` method
   - Can be prevented with `event.preventDefault()`

3. **ASAR Integration**:
   - File system wrapper in `lib/node/asar-fs-wrapper.ts`
   - Handles path resolution with `splitPath()` function
   - Caches archives with `getOrCreateArchive()`

### Test Infrastructure

The reproduction test case (`spec/pdf-asar-download-spec.ts`) includes:

1. **Basic PDF Download Test**: Tests will-download event with existing PDF files
2. **ASAR Path Resolution Test**: Tests download URL handling for ASAR paths
3. **PDF Viewer Initialization Test**: Tests loading PDFs from ASAR archives

## Reproduction Steps

### Test Case 1: Basic PDF Download from ASAR

```typescript
it('should trigger will-download event when PDF download is initiated from ASAR', async function () {
  const w = new BrowserWindow({ show: false });
  
  // Set up will-download event listener
  const willDownloadPromise = new Promise((resolve) => {
    w.webContents.session.once('will-download', (event, item, webContents) => {
      resolve({ event, item, webContents });
    });
  });

  // Create test page that triggers PDF download
  const testHtml = `
    <script>
      setTimeout(() => {
        const link = document.createElement('a');
        link.href = 'file://path/to/test.asar/document.pdf';
        link.download = 'test.pdf';
        document.body.appendChild(link);
        link.click();
      }, 500);
    </script>
  `;

  // Load test page and verify download event
  await w.loadFile(testHtmlPath);
  const result = await willDownloadPromise;
  
  // Verify download item properties
  expect(result.item.getURL()).to.include('.asar');
  expect(result.item.getFilename()).to.equal('test.pdf');
});
```

### Test Case 2: ASAR Path Resolution

```typescript
it('should properly handle ASAR path resolution in PDF downloads', async function () {
  const w = new BrowserWindow({ show: false });
  const asarPdfPath = path.join(asarDir, 'web.asar', 'test.pdf');
  
  const willDownloadPromise = new Promise((resolve) => {
    w.webContents.session.once('will-download', (event, item) => {
      event.preventDefault(); // Prevent actual download
      resolve({
        url: item.getURL(),
        filename: item.getFilename(),
        savePath: item.getSavePath()
      });
    });
  });

  // Trigger download using webContents.downloadURL
  w.webContents.downloadURL(`file://${asarPdfPath}`);
  
  const downloadDetails = await willDownloadPromise;
  
  // Verify ASAR path handling
  expect(downloadDetails.url).to.include('.asar');
  expect(downloadDetails.filename).to.equal('test.pdf');
});
```

## Expected Behavior

1. **will-download event should fire** when PDF downloads are initiated from ASAR archives
2. **File paths should be correctly resolved** from ASAR paths to actual file locations
3. **Download item properties** should contain accurate URL and filename information
4. **Event prevention** should work correctly to allow custom download handling

## Potential Issues to Investigate

1. **Path Resolution**: ASAR paths may not be properly resolved in the download system
2. **File Access**: PDF viewer may not correctly handle files within ASAR archives
3. **Event Timing**: will-download event may fire before ASAR path resolution completes
4. **Cross-Process Communication**: OOPIF PDF viewer may have issues communicating download events

## Files to Examine for Debugging

1. `shell/browser/api/electron_api_session.cc` - will-download event implementation
2. `lib/node/asar-fs-wrapper.ts` - ASAR file system integration
3. `shell/browser/electron_pdf_document_helper_client.cc` - PDF viewer integration
4. `shell/browser/extensions/api/pdf_viewer_private/pdf_viewer_private_api.cc` - PDF viewer API

## Running the Test

```bash
# Navigate to electron directory
cd electron

# Install dependencies (if needed)
npm install

# Run the specific test
npm test -- --grep "PDF viewer ASAR download integration"
```

## Notes

- Tests are conditionally run only when `isPDFViewerEnabled()` returns true
- Some tests may fail if ASAR archives don't contain actual PDF files (expected behavior)
- The test focuses on code path verification rather than actual file content