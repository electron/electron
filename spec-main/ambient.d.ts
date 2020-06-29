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
}

declare module 'dbus-native';
