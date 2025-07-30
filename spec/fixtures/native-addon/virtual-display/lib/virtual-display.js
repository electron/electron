module.exports = process.platform === 'darwin'
  ? require('../build/Release/virtual_display.node')
  : {
      addDisplay: () => { throw new Error('Virtual displays only supported on macOS'); },
      removeDisplay: () => { throw new Error('Virtual displays only supported on macOS'); }
    };
