const path = require('node:path');

const testLoadLibrary = require('./build/Release/test_module');

const lib = (() => {
  switch (process.platform) {
    case 'linux':
      return path.resolve(__dirname, 'build/Release/foo.so');
    case 'darwin':
      return path.resolve(__dirname, 'build/Release/foo.dylib');
    case 'win32':
      return path.resolve(__dirname, 'build/Release/libfoo.dll');
    default:
      throw new Error('unsupported os');
  }
})();

testLoadLibrary(lib);
