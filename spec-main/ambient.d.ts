declare var isCI: boolean;

declare namespace Electron {
  interface Menu {
    delegate: {
      executeCommand(menu: Menu, event: any, id: number): void;
      menuWillShow(menu: Menu): void;
    };
    getAcceleratorTextAt(index: number): string;
  }

  interface MenuItem {
    getDefaultRoleAccelerator(): Accelerator | undefined;
  }

  interface WebContents {
    getOwnerBrowserWindow(): BrowserWindow;
  }
}
