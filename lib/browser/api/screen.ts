const { createScreen } = process._linkedBinding('electron_common_screen');

let _screen: Electron.Screen;

// We can't call createScreen until after app.on('ready'), but this module
// exposes an instance created by createScreen. In order to avoid
// side-effecting and calling createScreen upon import of this module, instead
// we export a proxy which lazily calls createScreen on first access.
export default new Proxy({}, {
  get: (target, prop) => {
    if (_screen === undefined) {
      _screen = createScreen();
    }
    const v = (_screen as any)[prop];
    if (typeof v === 'function') {
      return v.bind(_screen);
    }
    return v;
  }
});
