# How to Run ASAR PDF Download Handler Verification

This guide walks you through verifying that the ASAR PDF download handler works correctly, including testing the Save As dialog, file writing, error handling, and edge cases.

## Quick Verification (Unit Tests)

Run the standalone unit tests to verify core functionality:

```bash
cd electron/test-app
node standalone-unit-tests.js
```

**Expected Output:**
```
🎉 ALL UNIT TESTS PASSED! (5/5)

✅ Core functionality verified:
  - ASAR URL parsing works correctly for all scenarios
  - PDF file detection works with extensions and MIME types
  - Edge cases are handled gracefully
  - Real-world scenarios work as expected
  - File operations work with mock ASAR structure
```

## Full Integration Testing

### Prerequisites

1. Install Electron in the test directory:
```bash
cd electron/test-app
npm install electron
```

### Method 1: Interactive Test Application

Run the interactive test application:

```bash
npm start
```

This opens a window with test buttons for:
- ✅ **ASAR PDF Download** - Tests save dialog and file writing
- ✅ **Regular PDF Download** - Verifies normal downloads still work  
- ✅ **Non-PDF ASAR Download** - Confirms only PDFs are intercepted
- ✅ **Error Cases** - Tests error handling and user feedback

### Method 2: Automated Integration Tests

Run the automated test suite:

```bash
electron automated-test.js
```

**Expected Output:**
```
🎉 All ASAR PDF Download Handler tests passed!
```

## Manual Verification Checklist

### ✅ Save Dialog Functionality

1. **Test ASAR PDF Download:**
   - Click "Download PDF from Mock ASAR" button
   - **VERIFY:** Save As dialog appears
   - **VERIFY:** Default filename is "test-asar-download.pdf"
   - **VERIFY:** File filters show "PDF Files" option
   - Select a save location and click Save
   - **VERIFY:** File is saved to selected location
   - **VERIFY:** No errors appear in console

2. **Test User Cancellation:**
   - Click "Download PDF from Mock ASAR" button
   - **VERIFY:** Save As dialog appears
   - Click Cancel in the dialog
   - **VERIFY:** No error messages appear
   - **VERIFY:** Console shows "User canceled save dialog"
   - **VERIFY:** No files are created

### ✅ File Writing Verification

1. **Check File Content:**
   - Download a PDF from ASAR using the test app
   - Navigate to the saved file location
   - **VERIFY:** File exists and has correct size (>400 bytes)
   - **VERIFY:** File opens correctly in PDF viewer
   - **VERIFY:** Content shows "Test PDF from ASAR" or similar

2. **Test Large Files:**
   - If testing with larger PDFs (>1MB)
   - **VERIFY:** Download completes without errors
   - **VERIFY:** File integrity is maintained

### ✅ Error Handling

1. **Test Non-Existent ASAR PDF:**
   - Click "Non-existent ASAR PDF" button
   - **VERIFY:** Error dialog appears with message about file not found
   - **VERIFY:** No crash or unhandled exceptions

2. **Test Permission Errors:**
   - Try saving to a read-only directory (if possible)
   - **VERIFY:** Appropriate error message is shown
   - **VERIFY:** User can retry with different location

### ✅ Regular Download Verification

1. **Test Non-ASAR Downloads:**
   - Click "Download Regular PDF" button
   - **VERIFY:** Normal browser download behavior occurs
   - **VERIFY:** Our handler does not interfere
   - **VERIFY:** File downloads to default location

2. **Test Non-PDF ASAR Files:**
   - Click "Download Non-PDF from ASAR" button
   - **VERIFY:** Normal download behavior (not intercepted)
   - **VERIFY:** No save dialog from our handler

## Console Output Verification

During testing, monitor the console for these expected messages:

### ✅ Successful ASAR PDF Download
```
Initializing ASAR PDF download handler...
ASAR PDF download handler initialized
will-download event fired for: file:///path/to/mock.asar/test.pdf
Intercepting ASAR PDF download: file:///path/to/mock.asar/test.pdf
Successfully saved PDF from ASAR: test.pdf
```

### ✅ User Cancellation
```
Intercepting ASAR PDF download: file:///path/to/mock.asar/test.pdf
User canceled save dialog for PDF: test.pdf
```

### ✅ Error Handling
```
Intercepting ASAR PDF download: file:///path/to/nonexistent.asar/test.pdf
Error handling ASAR PDF download: Failed to read file from ASAR: ENOENT: no such file or directory
```

### ✅ Regular Downloads (Not Intercepted)
```
will-download event fired for: https://example.com/regular.pdf
(No interception messages - normal download proceeds)
```

## Performance Verification

### ✅ Response Time
- **VERIFY:** Save dialog appears within 500ms of clicking download
- **VERIFY:** File writing completes quickly for typical PDFs (<5MB)
- **VERIFY:** No noticeable delay for non-ASAR downloads

### ✅ Memory Usage
- **VERIFY:** No memory leaks during multiple downloads
- **VERIFY:** Large PDFs don't cause excessive memory usage
- **VERIFY:** Cleanup occurs properly after downloads

## Troubleshooting

### Issue: Save Dialog Doesn't Appear
**Possible Causes:**
- Handler not initialized properly
- URL doesn't contain `.asar`
- File doesn't have `.pdf` extension

**Check:**
```javascript
// Verify handler is initialized
console.log('Handler initialized:', !!session.defaultSession.listenerCount('will-download'));

// Check URL parsing
const result = parseAsarUrl('your-test-url');
console.log('ASAR detected:', result.isAsar);
```

### Issue: File Not Written
**Possible Causes:**
- Permission denied in target directory
- Disk space insufficient
- File path contains invalid characters

**Check:**
- Try saving to a different location
- Check available disk space
- Verify write permissions

### Issue: Console Errors
**Common Errors:**
- `ENOENT: no such file or directory` - ASAR file doesn't exist
- `EACCES: permission denied` - No write permission
- `EMFILE: too many open files` - File handle leak

## Success Criteria

The verification is successful when:

- ✅ All unit tests pass (30/30)
- ✅ Save As dialog appears for ASAR PDFs
- ✅ Files are written correctly to disk
- ✅ User cancellation is handled gracefully
- ✅ Error conditions show appropriate dialogs
- ✅ Regular downloads continue working normally
- ✅ No console errors during normal operation
- ✅ Performance impact is minimal

## Final Verification Command

Run this complete verification sequence:

```bash
# 1. Unit tests
node standalone-unit-tests.js

# 2. Install dependencies
npm install electron

# 3. Interactive testing
npm start
# (Manually test all scenarios)

# 4. Automated integration tests
electron automated-test.js
```

If all steps complete successfully, the ASAR PDF Download Handler is verified and ready for production use.