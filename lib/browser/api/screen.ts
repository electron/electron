const { createScreen } = process._linkedBinding('electron_common_screen');

let _screen: Electron.Screen;

// We can't call createScreen until after app.on('ready'), but this module
// exposes an instance created by createScreen. In order to avoid
// side-effecting and calling createScreen upon import of this module, instead
// we export a proxy which lazily calls createScreen on first access.
export default new Proxy({}, {
  get: (target, prop: keyof Electron.Screen) => {
    if (_screen === undefined) {
      _screen = createScreen();
    }
    const v = _screen[prop];
    if (typeof v === 'function') {
      return v.bind(_screen);
    }
    return v;
  },
  ownKeys: () => {
    if (_screen === undefined) {
      _screen = createScreen();
    }
    return Reflect.ownKeys(_screen);
  },
  has: (target, prop: string) => {
    if (_screen === undefined) {
      _screen = createScreen();
    }
    return prop in _screen;
  },
  getOwnPropertyDescriptor: (target, prop: string) => {
    if (_screen === undefined) {
      _screen = createScreen();
    }
    return Reflect.getOwnPropertyDescriptor(_screen, prop);
  }
});
