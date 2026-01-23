#!/usr/bin/env node

const { spawn } = require('child_process');
const path = require('path');

console.log('🚀 Starting ASAR PDF Download Handler Tests...\n');

// Run the automated test
const testProcess = spawn('node', [path.join(__dirname, 'automated-test.js')], {
  stdio: 'inherit',
  env: { ...process.env, ELECTRON_RUN_AS_NODE: 'true' }
});

testProcess.on('close', (code) => {
  if (code === 0) {
    console.log('\n✅ All tests passed successfully!');
    console.log('\n📋 Test Summary:');
    console.log('- ✓ URL parsing for ASAR paths');
    console.log('- ✓ PDF file type detection');
    console.log('- ✓ Reading files from ASAR archives');
    console.log('- ✓ Save dialog functionality');
    console.log('- ✓ Session integration');
    console.log('- ✓ End-to-end download handling');
    console.log('\n🎯 Edge cases tested:');
    console.log('- ✓ User canceling save dialog');
    console.log('- ✓ Non-existent files in ASAR');
    console.log('- ✓ Non-PDF files from ASAR');
    console.log('- ✓ Regular downloads (not intercepted)');
    console.log('\n🔧 Manual Testing:');
    console.log('To run the interactive test application:');
    console.log('  cd test-app && npm install && npm start');
  } else {
    console.log(`\n❌ Tests failed with exit code ${code}`);
    console.log('Check the error messages above for details.');
  }
  
  process.exit(code);
});

testProcess.on('error', (error) => {
  console.error('Failed to start test process:', error);
  process.exit(1);
});