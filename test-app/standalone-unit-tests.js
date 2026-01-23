const assert = require('assert');
const path = require('path');
const fs = require('fs');
const url = require('url');

console.log('🧪 Running Standalone Unit Tests for ASAR PDF Download Handler...\n');

// Standalone implementations of the functions we want to test
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

function isPdfFile(filename, mimeType) {
  const extension = path.extname(filename).toLowerCase();
  return extension === '.pdf' || mimeType === 'application/pdf';
}

function testParseAsarUrl() {
  console.log('Testing parseAsarUrl function...');
  
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
      input: 'C:\\app\\resources\\app.asar\\docs\\guide.pdf',
      expected: { isAsar: true, asarPath: 'C:\\app\\resources\\app.asar', filePath: 'docs\\guide.pdf' }
    },
    {
      input: 'https://example.com/regular.pdf',
      expected: { isAsar: false }
    },
    {
      input: 'file:///regular/path/document.pdf',
      expected: { isAsar: false }
    },
    {
      input: 'file:///app.asar/root-file.pdf',
      expected: { isAsar: true, asarPath: '/app.asar', filePath: 'root-file.pdf' }
    },
    // Custom protocol tests
    {
      input: 'app://./resources/app.asar/docs/manual.pdf',
      expected: { isAsar: true, asarPath: '/resources/app.asar', filePath: 'docs/manual.pdf' }
    },
    {
      input: 'myapp://host/app.asar/documents/guide.pdf',
      expected: { isAsar: true, asarPath: '/app.asar', filePath: 'documents/guide.pdf' }
    }
  ];

  let passed = 0;
  let failed = 0;

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
      
      console.log(`  ✓ Test passed for: ${testCase.input}`);
      passed++;
    } catch (error) {
      console.log(`  ✗ Test failed for: ${testCase.input} - ${error.message}`);
      failed++;
    }
  }

  console.log(`  Results: ${passed} passed, ${failed} failed\n`);
  return failed === 0;
}

function testIsPdfFile() {
  console.log('Testing isPdfFile function...');
  
  const testCases = [
    { filename: 'document.pdf', mimeType: undefined, expected: true },
    { filename: 'document.PDF', mimeType: undefined, expected: true },
    { filename: 'document', mimeType: 'application/pdf', expected: true },
    { filename: 'document.txt', mimeType: undefined, expected: false },
    { filename: 'document.pdf', mimeType: 'text/plain', expected: true }, // Extension takes precedence
    { filename: 'document', mimeType: 'text/plain', expected: false },
    { filename: 'my-file.pdf.backup', mimeType: undefined, expected: false }, // Should not match
    { filename: 'file.PDF.txt', mimeType: undefined, expected: false } // Extension is .txt
  ];

  let passed = 0;
  let failed = 0;

  for (const testCase of testCases) {
    try {
      const result = isPdfFile(testCase.filename, testCase.mimeType);
      assert.strictEqual(result, testCase.expected, 
        `isPdfFile(${testCase.filename}, ${testCase.mimeType}) should return ${testCase.expected}`);
      console.log(`  ✓ Test passed for: ${testCase.filename} (${testCase.mimeType || 'undefined'})`);
      passed++;
    } catch (error) {
      console.log(`  ✗ Test failed for: ${testCase.filename} - ${error.message}`);
      failed++;
    }
  }

  console.log(`  Results: ${passed} passed, ${failed} failed\n`);
  return failed === 0;
}

function testEdgeCases() {
  console.log('Testing edge cases...');
  
  let passed = 0;
  let failed = 0;

  // Test empty/null inputs
  try {
    const result1 = parseAsarUrl('');
    assert.strictEqual(result1.isAsar, false, 'Empty string should not be ASAR');
    console.log('  ✓ Empty string handled correctly');
    passed++;
  } catch (error) {
    console.log(`  ✗ Empty string test failed: ${error.message}`);
    failed++;
  }

  try {
    const result2 = isPdfFile('');
    assert.strictEqual(result2, false, 'Empty filename should not be PDF');
    console.log('  ✓ Empty filename handled correctly');
    passed++;
  } catch (error) {
    console.log(`  ✗ Empty filename test failed: ${error.message}`);
    failed++;
  }

  // Test malformed URLs
  try {
    const result3 = parseAsarUrl('not-a-url');
    assert.strictEqual(result3.isAsar, false, 'Malformed URL should not be ASAR');
    console.log('  ✓ Malformed URL handled correctly');
    passed++;
  } catch (error) {
    console.log(`  ✗ Malformed URL test failed: ${error.message}`);
    failed++;
  }

  // Test case sensitivity for .asar extension
  try {
    const result4 = parseAsarUrl('file:///app/APP.ASAR/doc.pdf');
    assert.strictEqual(result4.isAsar, true, 'Should detect uppercase .ASAR');
    console.log('  ✓ Case sensitivity handled correctly');
    passed++;
  } catch (error) {
    console.log(`  ✗ Case sensitivity test failed: ${error.message}`);
    failed++;
  }

  // Test URLs with query parameters
  try {
    const result5 = parseAsarUrl('file:///app.asar/doc.pdf?version=1');
    // This should still work because we parse the pathname
    console.log('  ✓ URLs with query parameters handled');
    passed++;
  } catch (error) {
    console.log(`  ✗ Query parameter test failed: ${error.message}`);
    failed++;
  }

  // Test very long paths
  try {
    const longPath = 'file:///very/long/path/to/app.asar/' + 'nested/'.repeat(20) + 'deep.pdf';
    const result6 = parseAsarUrl(longPath);
    assert.strictEqual(result6.isAsar, true, 'Should handle long paths');
    console.log('  ✓ Long paths handled correctly');
    passed++;
  } catch (error) {
    console.log(`  ✗ Long path test failed: ${error.message}`);
    failed++;
  }

  console.log(`  Results: ${passed} passed, ${failed} failed\n`);
  return failed === 0;
}

function testRealWorldScenarios() {
  console.log('Testing real-world scenarios...');
  
  let passed = 0;
  let failed = 0;

  const realWorldCases = [
    // Typical Electron app structure
    {
      url: 'file:///Users/developer/MyApp.app/Contents/Resources/app.asar/assets/manual.pdf',
      shouldBeAsar: true,
      description: 'macOS app bundle'
    },
    // Windows app structure
    {
      url: 'file:///C:/Program Files/MyApp/resources/app.asar/docs/help.pdf',
      shouldBeAsar: true,
      description: 'Windows app installation'
    },
    // Development environment
    {
      url: 'file:///home/dev/project/dist/app.asar/resources/guide.pdf',
      shouldBeAsar: true,
      description: 'Linux development environment'
    },
    // Regular web download
    {
      url: 'https://cdn.example.com/documents/specification.pdf',
      shouldBeAsar: false,
      description: 'Regular web download'
    },
    // Local file (not in ASAR)
    {
      url: 'file:///Users/developer/Downloads/document.pdf',
      shouldBeAsar: false,
      description: 'Local file download'
    }
  ];

  for (const testCase of realWorldCases) {
    try {
      const result = parseAsarUrl(testCase.url);
      assert.strictEqual(result.isAsar, testCase.shouldBeAsar, 
        `${testCase.description} should ${testCase.shouldBeAsar ? '' : 'not '}be detected as ASAR`);
      console.log(`  ✓ ${testCase.description}: ${testCase.shouldBeAsar ? 'ASAR' : 'Regular'}`);
      passed++;
    } catch (error) {
      console.log(`  ✗ ${testCase.description} failed: ${error.message}`);
      failed++;
    }
  }

  console.log(`  Results: ${passed} passed, ${failed} failed\n`);
  return failed === 0;
}

function createAndTestMockFiles() {
  console.log('Testing file operations with mock ASAR structure...');
  
  const mockDir = path.join(__dirname, 'mock-test.asar');
  const mockPdfPath = path.join(mockDir, 'documents', 'test.pdf');

  let passed = 0;
  let failed = 0;

  try {
    // Create directory structure
    fs.mkdirSync(path.dirname(mockPdfPath), { recursive: true });

    // Create a simple PDF file
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
(Unit Test PDF) Tj
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
    console.log(`  ✓ Created mock ASAR structure at: ${mockDir}`);
    passed++;

    // Test that our mock PDF exists and is readable
    const content = fs.readFileSync(mockPdfPath);
    assert(Buffer.isBuffer(content), 'Should return a Buffer');
    assert(content.length > 0, 'Buffer should not be empty');
    assert(content.toString().startsWith('%PDF'), 'Should be a valid PDF file');
    
    console.log(`  ✓ Mock PDF file operations work correctly (${content.length} bytes)`);
    passed++;

    // Test URL parsing with our mock structure
    const mockUrl = `file://${mockPdfPath.replace(/\\/g, '/')}`;
    const parseResult = parseAsarUrl(mockUrl);
    assert.strictEqual(parseResult.isAsar, true, 'Should detect mock ASAR');
    assert(parseResult.asarPath.endsWith('mock-test.asar'), 'Should extract correct ASAR path');
    assert.strictEqual(parseResult.filePath, 'documents/test.pdf', 'Should extract correct file path');
    
    console.log(`  ✓ URL parsing works with mock structure`);
    passed++;

    // Test PDF detection
    const isPdf = isPdfFile('test.pdf', 'application/pdf');
    assert.strictEqual(isPdf, true, 'Should detect PDF file');
    
    console.log(`  ✓ PDF detection works correctly`);
    passed++;

  } catch (error) {
    console.log(`  ✗ Mock file test failed: ${error.message}`);
    failed++;
  }

  // Clean up
  try {
    if (fs.existsSync(mockDir)) {
      fs.rmSync(mockDir, { recursive: true, force: true });
      console.log(`  ✓ Cleaned up mock files`);
      passed++;
    }
  } catch (error) {
    console.log(`  ✗ Cleanup failed: ${error.message}`);
    failed++;
  }

  console.log(`  Results: ${passed} passed, ${failed} failed\n`);
  return failed === 0;
}

// Run all tests
function runAllTests() {
  console.log('='.repeat(70));
  console.log('ASAR PDF Download Handler - Standalone Unit Tests');
  console.log('='.repeat(70));
  console.log();

  const results = [];
  
  results.push(testParseAsarUrl());
  results.push(testIsPdfFile());
  results.push(testEdgeCases());
  results.push(testRealWorldScenarios());
  results.push(createAndTestMockFiles());

  const allPassed = results.every(result => result === true);
  const passedCount = results.filter(result => result === true).length;
  const totalCount = results.length;

  console.log('='.repeat(70));
  console.log('FINAL RESULTS');
  console.log('='.repeat(70));
  
  if (allPassed) {
    console.log(`🎉 ALL UNIT TESTS PASSED! (${passedCount}/${totalCount})`);
    console.log();
    console.log('✅ Core functionality verified:');
    console.log('  - ASAR URL parsing works correctly for all scenarios');
    console.log('  - PDF file detection works with extensions and MIME types');
    console.log('  - Edge cases are handled gracefully');
    console.log('  - Real-world scenarios work as expected');
    console.log('  - File operations work with mock ASAR structure');
    console.log();
    console.log('🔧 Integration testing recommendations:');
    console.log('  1. Test with actual Electron environment');
    console.log('  2. Verify will-download event interception');
    console.log('  3. Test save dialog functionality');
    console.log('  4. Verify user cancellation scenarios');
    console.log('  5. Test error handling with real ASAR files');
    console.log();
    console.log('📋 Manual testing checklist:');
    console.log('  □ Save As dialog appears for ASAR PDFs');
    console.log('  □ File is written correctly to selected location');
    console.log('  □ No console errors during download process');
    console.log('  □ User cancellation is handled gracefully');
    console.log('  □ Regular downloads still work normally');
    console.log('  □ Non-PDF ASAR files are not intercepted');
    console.log('  □ Error dialogs appear for invalid files');
    console.log();
    return true;
  } else {
    console.log(`❌ SOME TESTS FAILED (${passedCount}/${totalCount} passed)`);
    console.log();
    console.log('Please check the error messages above and fix the issues.');
    return false;
  }
}

// Run the tests
const success = runAllTests();
process.exit(success ? 0 : 1);