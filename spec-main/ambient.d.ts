declare let standardScheme: string;

declare namespace Electron {
  interface Menu {
    _executeCommand(event: any, id: number): void;
    _menuWillShow(): void;
    getAcceleratorTextAt(index: number): string;
  }

  interface MenuItem {
    getDefaultRoleAccelerator(): Accelerator | undefined;
  }

  interface WebContents {
    getOwnerBrowserWindow(): BrowserWindow;
    getWebPreferences(): any;
  }

  interface Session {
    destroy(): void;
  }

  // Experimental views API
  class TopLevelWindow {
    constructor(args: {show: boolean})
    setContentView(view: View): void
  }
  class WebContentsView {
    constructor(options: BrowserWindowConstructorOptions)
  }

  namespace Main {
    class TopLevelWindow extends Electron.TopLevelWindow {}
    class View extends Electron.View {}
    class WebContentsView extends Electron.WebContentsView {}
  }
}

declare module 'dbus-native';
