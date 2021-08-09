declare let standardScheme: string;

declare namespace Electron {
  interface WebContents {
    getOwnerBrowserWindow(): BrowserWindow | null;
    getWebPreferences(): WebPreferences | null;
  }

  interface Session {
    destroy(): void;
  }
}

declare module 'dbus-native';
