const assert = require('assert');
const path = require('path');
const fs = require('fs');

// Import just the utility functions that don't require Electron
const { parseAsarUrl, isPdfFile } = require('../lib/browser/asar-pdf-download-handler.js');

console.log('🧪 Running Unit Tests for ASAR PDF Download Handler...\n');

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
    { filename: 'document', mimeType: 'text/plain', expected: false }
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

  // Test case sensitivity
  try {
    const result4 = parseAsarUrl('file:///app/APP.ASAR/doc.pdf');
    assert.strictEqual(result4.isAsar, true, 'Should detect uppercase .ASAR');
    console.log('  ✓ Case sensitivity handled correctly');
    passed++;
  } catch (error) {
    console.log(`  ✗ Case sensitivity test failed: ${error.message}`);
    failed++;
  }

  console.log(`  Results: ${passed} passed, ${failed} failed\n`);
  return failed === 0;
}

function createMockFiles() {
  console.log('Creating mock files for testing...');
  
  const mockDir = path.join(__dirname, 'mock-test.asar');
  const mockPdfPath = path.join(mockDir, 'test.pdf');

  try {
    // Create directory
    fs.mkdirSync(mockDir, { recursive: true });

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
    console.log(`  ✓ Created mock PDF at: ${mockPdfPath}`);
    
    return { mockDir, mockPdfPath };
  } catch (error) {
    console.log(`  ✗ Failed to create mock files: ${error.message}`);
    return null;
  }
}

function testFileOperations() {
  console.log('Testing file operations...');
  
  const mockFiles = createMockFiles();
  if (!mockFiles) {
    console.log('  ✗ Cannot test file operations without mock files\n');
    return false;
  }

  let passed = 0;
  let failed = 0;

  try {
    // Test that our mock PDF exists and is readable
    const content = fs.readFileSync(mockFiles.mockPdfPath);
    assert(Buffer.isBuffer(content), 'Should return a Buffer');
    assert(content.length > 0, 'Buffer should not be empty');
    assert(content.toString().startsWith('%PDF'), 'Should be a valid PDF file');
    
    console.log(`  ✓ Mock PDF file operations work correctly`);
    passed++;
  } catch (error) {
    console.log(`  ✗ File operations test failed: ${error.message}`);
    failed++;
  }

  // Clean up
  try {
    fs.rmSync(mockFiles.mockDir, { recursive: true, force: true });
    console.log(`  ✓ Cleaned up mock files`);
    passed++;
  } catch (error) {
    console.log(`  ✗ Cleanup failed: ${error.message}`);
    failed++;
  }

  console.log(`  Results: ${passed} passed, ${failed} failed\n`);
  return failed === 0;
}

// Run all tests
function runAllTests() {
  console.log('='.repeat(60));
  console.log('ASAR PDF Download Handler - Unit Tests');
  console.log('='.repeat(60));
  console.log();

  const results = [];
  
  results.push(testParseAsarUrl());
  results.push(testIsPdfFile());
  results.push(testEdgeCases());
  results.push(testFileOperations());

  const allPassed = results.every(result => result === true);
  const passedCount = results.filter(result => result === true).length;
  const totalCount = results.length;

  console.log('='.repeat(60));
  console.log('FINAL RESULTS');
  console.log('='.repeat(60));
  
  if (allPassed) {
    console.log(`🎉 ALL TESTS PASSED! (${passedCount}/${totalCount})`);
    console.log();
    console.log('✅ Core functionality verified:');
    console.log('  - ASAR URL parsing works correctly');
    console.log('  - PDF file detection works correctly');
    console.log('  - Edge cases are handled gracefully');
    console.log('  - File operations work as expected');
    console.log();
    console.log('🔧 Next steps for full verification:');
    console.log('  1. Run the interactive test app: npm start');
    console.log('  2. Test with real Electron environment');
    console.log('  3. Verify save dialog functionality');
    console.log('  4. Test user cancellation scenarios');
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