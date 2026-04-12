import { BrowserWindow, WebContentsView } from 'electron/main';

import { expect } from 'chai';
import { afterEach, describe, it } from 'vitest';

import { once } from 'node:events';

import { closeAllWindows } from '../lib/window-helpers';

describe('WebContentsView (serial)', () => {
  afterEach(closeAllWindows);

  describe('focusOnNavigation webPreference', () => {
    it('focuses the webContents on navigation by default', async () => {
      const w = new BrowserWindow();
      await once(w, 'focus');
      const v = new WebContentsView();
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');
      const devToolsFocused = once(v.webContents, 'devtools-focused');
      v.webContents.openDevTools({ mode: 'right' });
      await devToolsFocused;
      expect(v.webContents.isFocused()).to.be.false();
      await v.webContents.loadURL('data:text/html,<body>test</body>');
      expect(v.webContents.isFocused()).to.be.true();
    });

    it('does not focus the webContents on navigation when focusOnNavigation is false', async () => {
      const w = new BrowserWindow();
      await once(w, 'focus');
      const v = new WebContentsView({
        webPreferences: {
          focusOnNavigation: false
        }
      });
      w.setContentView(v);
      await v.webContents.loadURL('about:blank');
      const devToolsFocused = once(v.webContents, 'devtools-focused');
      v.webContents.openDevTools({ mode: 'right' });
      await devToolsFocused;
      expect(v.webContents.isFocused()).to.be.false();
      await v.webContents.loadURL('data:text/html,<body>test</body>');
      expect(v.webContents.isFocused()).to.be.false();
    });
  });
});
