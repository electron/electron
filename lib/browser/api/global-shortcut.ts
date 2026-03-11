const { createGlobalShortcut } = process._linkedBinding('electron_browser_global_shortcut');

let globalShortcut: Electron.GlobalShortcut;

const createGlobalShortcutIfNeeded = () => {
  if (globalShortcut === undefined) {
    globalShortcut = createGlobalShortcut();
  }
};

export default new Proxy({}, {
  get: (_target, property: keyof Electron.GlobalShortcut) => {
    createGlobalShortcutIfNeeded();
    const value = globalShortcut[property];
    if (typeof value === 'function') {
      return value.bind(globalShortcut);
    }
    return value;
  },
  set: (_target, property: string, value: unknown) => {
    createGlobalShortcutIfNeeded();
    return Reflect.set(globalShortcut, property, value);
  },
  ownKeys: () => {
    createGlobalShortcutIfNeeded();
    return Reflect.ownKeys(globalShortcut);
  },
  has: (_target, property: string) => {
    createGlobalShortcutIfNeeded();
    return property in globalShortcut;
  },
  getOwnPropertyDescriptor: (_target, property: string) => {
    createGlobalShortcutIfNeeded();
    return Reflect.getOwnPropertyDescriptor(globalShortcut, property);
  }
});
