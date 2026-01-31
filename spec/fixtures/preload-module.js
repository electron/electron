// Test ESM preload script for webview with type="module"
console.log('Preload script loaded with type=module support');

// Test ES6 import syntax (this should work if type=module is properly supported)
// For now, just test that the script loads
window.moduleTypeSupported = true;