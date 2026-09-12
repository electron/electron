// menu.ts adds Menu's remaining JS methods; a MenuItem can hand out a Menu
// (its submenu) before anything else has loaded it.
import '@electron/internal/browser/api/menu';

const { MenuItem } = process._linkedBinding('electron_browser_menu');

module.exports = MenuItem;
