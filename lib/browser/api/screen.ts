import { EventEmitter } from 'events';

const { createScreen } = process._linkedBinding('electron_browser_screen');

let _screen: Electron.Screen;

const createScreenIfNeeded = () => {
  if (_screen === undefined) {
    _screen = createScreen();
  }
};

// We can't call createScreen until after app.on('ready'), but this module
// exposes an instance created by createScreen. In order to avoid
// side-effecting and calling createScreen upon import of this module, instead
// we export a proxy which lazily calls createScreen on first access.
export default new Proxy({}, {
  get: (target, property: keyof Electron.Screen) => {
    createScreenIfNeeded();
    const value = _screen[property];
    if (typeof value === 'function') {
      return value.bind(_screen);
    }
    return value;
  },
  set: (target, property: string, value: unknown) => {
    createScreenIfNeeded();
    return Reflect.set(_screen, property, value);
  },
  ownKeys: () => {
    createScreenIfNeeded();
    return Reflect.ownKeys(_screen);
  },
  has: (target, property: string) => {
    createScreenIfNeeded();
    return property in _screen;
  },
  getOwnPropertyDescriptor: (target, property: string) => {
    createScreenIfNeeded();
    return Reflect.getOwnPropertyDescriptor(_screen, property);
  },
  getPrototypeOf: () => {
    // This is necessary as a result of weirdness with EventEmitterMixin
    // and FunctionTemplate - we need to explicitly ensure it's returned
    // in the prototype.
    return EventEmitter.prototype;
  }
});
