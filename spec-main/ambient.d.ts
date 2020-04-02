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
  class View {}
  class WebContentsView {
    constructor(webContents: WebContents)
  }
}

declare module 'dbus-native';
