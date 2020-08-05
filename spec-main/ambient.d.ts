declare let standardScheme: string;

declare namespace Electron {
  interface WebContents {
    getOwnerBrowserWindow(): BrowserWindow;
    getWebPreferences(): any;
  }

  interface Session {
    destroy(): void;
  }
}

declare module 'dbus-native';
