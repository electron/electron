import { TouchBar } from 'electron/main';
import type { BaseWindow as TLWT } from 'electron/main';

const { BaseWindow } = process._linkedBinding('electron_browser_base_window') as { BaseWindow: typeof TLWT };

BaseWindow.prototype.setTouchBar = function (touchBar) {
  (TouchBar as any)._setOnWindow(touchBar, this);
};

module.exports = BaseWindow;
