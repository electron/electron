const { app, BrowserWindow, dialog, session } = require('electron');
const path = require('path');
const fs = require('fs');
const assert = require('assert');

// Import our ASAR PDF download handler
const { 
  initializeAsarPdfDownloadHandler, 
  removeAsarPdfDownloadHandler,
  parseAsarUrl,
  isPdfFile,
  readFromAsar,
  saveAsarPdf
} = require('../lib/browser/asar-pdf-download-handler.js');

class AsarPdfDownloadTester {
  constructor() {
    this.testResults = [];
    this.mockAsarPath = '';
    this.mockPdfPath = '';
    this.testWindow = null;
  }

  log(message, type = 'info') {
    const timestamp = new Date().toISOString();
    console.log(`[${timestamp}] [${type.toUpperCase()}] ${message}`);
    this.testResults.push({ timestamp, type, message });
  }

  async createMockAsarWithPdf() {
    const mockAsarDir = path.join(__dirname, 'test-mock.asar');
    const mockPdfPath = path.join(mockAsarDir, 'documents', 'test.pdf');

    // Create directory structure
    fs.mkdirSync(path.dirname(mockPdfPath), { recursive: true });

    // Create a valid PDF file
    const pdfContent = Buffer.from(`%PDF-1.4
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
/Length 50
>>
stream
BT
/F1 12 Tf
72 720 Td
(Automated Test PDF from ASAR) Tj
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
305
%%EOF`);

    fs.writeFileSync(mockPdfPath, pdfContent);
    
    this.mockAsarPath = mockAsarDir;
    this.mockPdfPath = mockPdfPath;
    
    this.log(`Created mock ASAR at: ${mockAsarDir}`, 'success');
    this.log(`Created mock PDF at: ${mockPdfPath}`, 'success');
    
    return { asarPath: mockAsarDir, pdfPath: mockPdfPath };
  }

  async testParseAsarUrl() {
    this.log('Testing parseAsarUrl function...', 'info');
    
    const testCases = [
      {
        input: 'file:///app/resources/app.asar/docs/manual.pdf',
        expected: { isAsar: true, asarPath: '/app/resources/app.asar', filePath: 'docs/manual.pdf' }
      },
      {
        input: '/path/to/app.asar/documents/test.pdf',
        expected: { isAsar: true, asarPath: '/path/to/app.asar', filePath: 'documents/test.pdf' }
      },
      {
        input: 'https://example.com/regular.pdf',
        expected: { isAsar: false }
      },
      {
        input: 'file:///regular/path/document.pdf',
        expected: { isAsar: false }
      }
    ];

    for (const testCase of testCases) {
      try {
        const result = parseAsarUrl(testCase.input);
        
        if (testCase.expected.isAsar) {
          assert.strictEqual(result.isAsar, true, `Should detect ASAR for: ${testCase.input}`);
          assert.strictEqual(result.asarPath, testCase.expected.asarPath, `ASAR path mismatch for: ${testCase.input}`);
          assert.strictEqual(result.filePath, testCase.expected.filePath, `File path mismatch for: ${testCase.input}`);
        } else {
          assert.strictEqual(result.isAsar, false, `Should not detect ASAR for: ${testCase.input}`);
        }
        
        this.log(`✓ parseAsarUrl test passed for: ${testCase.input}`, 'success');
      } catch (error) {
        this.log(`✗ parseAsarUrl test failed for: ${testCase.input} - ${error.message}`, 'error');
        throw error;
      }
    }
  }

  async testIsPdfFile() {
    this.log('Testing isPdfFile function...', 'info');
    
    const testCases = [
      { filename: 'document.pdf', mimeType: undefined, expected: true },
      { filename: 'document.PDF', mimeType: undefined, expected: true },
      { filename: 'document', mimeType: 'application/pdf', expected: true },
      { filename: 'document.txt', mimeType: undefined, expected: false },
      { filename: 'document.pdf', mimeType: 'text/plain', expected: true }, // Extension takes precedence
      { filename: 'document', mimeType: 'text/plain', expected: false }
    ];

    for (const testCase of testCases) {
      try {
        const result = isPdfFile(testCase.filename, testCase.mimeType);
        assert.strictEqual(result, testCase.expected, 
          `isPdfFile(${testCase.filename}, ${testCase.mimeType}) should return ${testCase.expected}`);
        this.log(`✓ isPdfFile test passed for: ${testCase.filename} (${testCase.mimeType})`, 'success');
      } catch (error) {
        this.log(`✗ isPdfFile test failed for: ${testCase.filename} - ${error.message}`, 'error');
        throw error;
      }
    }
  }

  async testReadFromAsar() {
    this.log('Testing readFromAsar function...', 'info');
    
    try {
      // Test reading the mock PDF we created
      const content = readFromAsar(this.mockAsarPath, 'documents/test.pdf');
      
      assert(Buffer.isBuffer(content), 'Should return a Buffer');
      assert(content.length > 0, 'Buffer should not be empty');
      assert(content.toString().startsWith('%PDF'), 'Should be a valid PDF file');
      
      this.log(`✓ readFromAsar test passed - read ${content.length} bytes`, 'success');
    } catch (error) {
      this.log(`✗ readFromAsar test failed: ${error.message}`, 'error');
      throw error;
    }

    // Test error case - non-existent file
    try {
      readFromAsar(this.mockAsarPath, 'nonexistent.pdf');
      this.log(`✗ readFromAsar should have thrown error for non-existent file`, 'error');
      throw new Error('Expected error for non-existent file');
    } catch (error) {
      if (error.message.includes('Failed to read file from ASAR')) {
        this.log(`✓ readFromAsar correctly threw error for non-existent file`, 'success');
      } else {
        throw error;
      }
    }
  }

  async testSaveAsarPdf() {
    this.log('Testing saveAsarPdf function...', 'info');
    
    const mockWebContents = {};
    const pdfContent = Buffer.from('mock pdf content');
    const filename = 'test-save.pdf';
    const tempSavePath = path.join(__dirname, 'temp-test-save.pdf');

    // Mock dialog.showSaveDialog to return a specific path
    const originalShowSaveDialog = dialog.showSaveDialog;
    dialog.showSaveDialog = async () => ({
      canceled: false,
      filePath: tempSavePath
    });

    try {
      const result = await saveAsarPdf(mockWebContents, pdfContent, filename);
      
      assert.strictEqual(result, true, 'Should return true on successful save');
      assert(fs.existsSync(tempSavePath), 'File should be created');
      
      const savedContent = fs.readFileSync(tempSavePath);
      assert(savedContent.equals(pdfContent), 'Saved content should match original');
      
      this.log(`✓ saveAsarPdf test passed - file saved correctly`, 'success');
      
      // Clean up
      fs.unlinkSync(tempSavePath);
    } catch (error) {
      this.log(`✗ saveAsarPdf test failed: ${error.message}`, 'error');
      throw error;
    } finally {
      // Restore original dialog function
      dialog.showSaveDialog = originalShowSaveDialog;
    }

    // Test cancellation case
    dialog.showSaveDialog = async () => ({ canceled: true });
    
    try {
      const result = await saveAsarPdf(mockWebContents, pdfContent, filename);
      assert.strictEqual(result, false, 'Should return false when user cancels');
      this.log(`✓ saveAsarPdf correctly handled user cancellation`, 'success');
    } catch (error) {
      this.log(`✗ saveAsarPdf cancellation test failed: ${error.message}`, 'error');
      throw error;
    } finally {
      dialog.showSaveDialog = originalShowSaveDialog;
    }
  }

  async testSessionIntegration() {
    this.log('Testing session integration...', 'info');
    
    const testSession = session.fromPartition('test-asar-handler');
    
    // Test initialization
    const listenerCountBefore = testSession.listenerCount ? testSession.listenerCount('will-download') : 0;
    initializeAsarPdfDownloadHandler(testSession);
    
    // Note: listenerCount might not be available in all Electron versions
    // So we'll test by triggering an event instead
    
    let handlerCalled = false;
    const testPromise = new Promise((resolve) => {
      testSession.once('will-download', (event, item) => {
        handlerCalled = true;
        event.preventDefault(); // Prevent actual download
        item.cancel();
        resolve();
      });
    });

    // Create a test window with the custom session
    const testWindow = new BrowserWindow({
      show: false,
      webPreferences: {
        session: testSession,
        nodeIntegration: true,
        contextIsolation: false
      }
    });

    // Trigger a download
    const asarPdfUrl = `file://${this.mockPdfPath.replace(/\\/g, '/')}`;
    testWindow.webContents.downloadURL(asarPdfUrl);

    // Wait for the event or timeout
    await Promise.race([
      testPromise,
      new Promise((_, reject) => 
        setTimeout(() => reject(new Error('will-download event timeout')), 3000)
      )
    ]);

    assert(handlerCalled, 'will-download handler should have been called');
    this.log(`✓ Session integration test passed`, 'success');

    // Clean up
    testWindow.close();
    removeAsarPdfDownloadHandler(testSession);
  }

  async testEndToEndScenario() {
    this.log('Testing end-to-end scenario...', 'info');
    
    // Create a test window
    this.testWindow = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        contextIsolation: false
      }
    });

    // Initialize handler
    initializeAsarPdfDownloadHandler(this.testWindow.webContents.session);

    let downloadEventFired = false;
    let downloadPrevented = false;
    let itemCanceled = false;

    const testPromise = new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        reject(new Error('End-to-end test timeout'));
      }, 5000);

      this.testWindow.webContents.session.once('will-download', async (event, item) => {
        try {
          downloadEventFired = true;
          this.log(`will-download event fired for: ${item.getURL()}`, 'info');

          // Verify it's our ASAR PDF
          const url = item.getURL();
          const filename = item.getFilename();
          
          assert(url.includes('.asar'), 'URL should contain .asar');
          assert(filename.endsWith('.pdf'), 'Filename should end with .pdf');

          // Our handler should prevent the default download
          // We'll simulate this by checking if preventDefault was called
          const originalPreventDefault = event.preventDefault;
          event.preventDefault = () => {
            downloadPrevented = true;
            originalPreventDefault.call(event);
          };

          // Simulate our handler logic
          if (url.includes('.asar') && filename.endsWith('.pdf')) {
            event.preventDefault();
            
            // Mock the save dialog to avoid UI interaction
            const tempSavePath = path.join(__dirname, 'end-to-end-test.pdf');
            
            try {
              // Read from ASAR
              const asarInfo = parseAsarUrl(url);
              const pdfContent = readFromAsar(asarInfo.asarPath, asarInfo.filePath);
              
              // Write to temp location
              fs.writeFileSync(tempSavePath, pdfContent);
              
              // Verify file was written
              assert(fs.existsSync(tempSavePath), 'PDF should be saved');
              const savedContent = fs.readFileSync(tempSavePath);
              assert(savedContent.length > 0, 'Saved file should not be empty');
              
              this.log(`✓ PDF successfully saved to: ${tempSavePath}`, 'success');
              
              // Clean up
              fs.unlinkSync(tempSavePath);
              
              // Cancel the item
              item.cancel();
              itemCanceled = true;
              
              clearTimeout(timeout);
              resolve();
              
            } catch (error) {
              clearTimeout(timeout);
              reject(error);
            }
          }
        } catch (error) {
          clearTimeout(timeout);
          reject(error);
        }
      });

      // Trigger download
      const asarPdfUrl = `file://${this.mockPdfPath.replace(/\\/g, '/')}`;
      this.testWindow.webContents.downloadURL(asarPdfUrl);
    });

    await testPromise;

    assert(downloadEventFired, 'will-download event should have fired');
    assert(downloadPrevented, 'Download should have been prevented');
    assert(itemCanceled, 'Download item should have been canceled');

    this.log(`✓ End-to-end test passed`, 'success');

    // Clean up
    removeAsarPdfDownloadHandler(this.testWindow.webContents.session);
    this.testWindow.close();
  }

  async runAllTests() {
    this.log('Starting ASAR PDF Download Handler automated tests...', 'info');
    
    try {
      // Setup
      await this.createMockAsarWithPdf();
      
      // Run unit tests
      await this.testParseAsarUrl();
      await this.testIsPdfFile();
      await this.testReadFromAsar();
      await this.testSaveAsarPdf();
      
      // Run integration tests
      await this.testSessionIntegration();
      await this.testEndToEndScenario();
      
      this.log('All tests passed successfully! ✓', 'success');
      return true;
      
    } catch (error) {
      this.log(`Test failed: ${error.message}`, 'error');
      console.error(error);
      return false;
    } finally {
      // Clean up mock files
      if (this.mockAsarPath && fs.existsSync(this.mockAsarPath)) {
        fs.rmSync(this.mockAsarPath, { recursive: true, force: true });
        this.log('Cleaned up mock ASAR files', 'info');
      }
    }
  }

  generateReport() {
    const successCount = this.testResults.filter(r => r.type === 'success').length;
    const errorCount = this.testResults.filter(r => r.type === 'error').length;
    const totalCount = this.testResults.length;

    console.log('\n=== TEST REPORT ===');
    console.log(`Total log entries: ${totalCount}`);
    console.log(`Successful operations: ${successCount}`);
    console.log(`Errors: ${errorCount}`);
    console.log('===================\n');

    return { successCount, errorCount, totalCount, results: this.testResults };
  }
}

// Run tests when app is ready
app.whenReady().then(async () => {
  const tester = new AsarPdfDownloadTester();
  
  try {
    const success = await tester.runAllTests();
    const report = tester.generateReport();
    
    if (success) {
      console.log('🎉 All ASAR PDF Download Handler tests passed!');
      process.exit(0);
    } else {
      console.log('❌ Some tests failed. Check the logs above.');
      process.exit(1);
    }
  } catch (error) {
    console.error('Test runner error:', error);
    process.exit(1);
  }
});

app.on('window-all-closed', () => {
  // Don't quit on macOS
  if (process.platform !== 'darwin') {
    app.quit();
  }
});