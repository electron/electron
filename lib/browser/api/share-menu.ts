import { BrowserWindow, Menu, SharingItem, PopupOptions } from 'electron/main';

class ShareMenu {
  private menu: Menu;

  constructor (sharingItem: SharingItem) {
    this.menu = new (Menu as any)({ sharingItem });
  }

  popup (options?: PopupOptions) {
    this.menu.popup(options);
  }

  closePopup (browserWindow?: BrowserWindow) {
    this.menu.closePopup(browserWindow);
  }
}

export default ShareMenu;
