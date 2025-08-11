module.exports = process.platform === 'darwin'
  ? require('../build/Release/virtual_display.node')
  : {
      create: () => { throw new Error('Virtual displays only supported on macOS'); },
      destroy: () => { throw new Error('Virtual displays only supported on macOS'); }
    };
