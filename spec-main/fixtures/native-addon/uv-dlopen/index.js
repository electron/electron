const testLoadLibrary = require('./build/Release/test_module');

const lib = (() => {
  switch (process.platform) {
    case 'linux':
      return `${__dirname}/build/Release/foo.so`;
    case 'win32':
      return `${__dirname}/build/Release/libfoo.dll`;
    // TODO: add library path for mac
    default:
      throw new Error('unsupported os');
  }
})();

testLoadLibrary(lib);
