import { BrowserView, BrowserWindow, screen } from 'electron/main';

import { afterEach, beforeEach, describe } from 'vitest';

import { ScreenCapture, hasCapturableScreen } from '../lib/screen-helpers';
import { ifit } from '../lib/spec-helpers';
import { closeAllWindows } from '../lib/window-helpers';

describe('BrowserView.setBackgroundColor() (serial)', () => {
  let w: BrowserWindow;
  let view: BrowserView;

  beforeEach(() => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: { backgroundThrottling: false }
    });
  });

  afterEach(async () => {
    if (view?.webContents && !view.webContents.isDestroyed()) {
      view.webContents.destroy();
    }
    view = null as unknown as BrowserView;
    await closeAllWindows();
  });

  ifit(hasCapturableScreen())('sets the background color to transparent if none is set', async () => {
    const display = screen.getPrimaryDisplay();
    const WINDOW_BACKGROUND_COLOR = '#55ccbb';

    w.show();
    w.setBounds(display.bounds);
    w.setBackgroundColor(WINDOW_BACKGROUND_COLOR);
    await w.loadURL('data:text/html,<html></html>');

    view = new BrowserView();
    view.setBounds(display.bounds);
    w.setBrowserView(view);
    await view.webContents.loadURL('data:text/html,hello there');

    const screenCapture = new ScreenCapture(display);
    await screenCapture.expectColorAtCenterMatches(WINDOW_BACKGROUND_COLOR);
  });

  ifit(hasCapturableScreen())('successfully applies the background color', async () => {
    const WINDOW_BACKGROUND_COLOR = '#55ccbb';
    const VIEW_BACKGROUND_COLOR = '#ff00ff';
    const display = screen.getPrimaryDisplay();

    w.show();
    w.setBounds(display.bounds);
    w.setBackgroundColor(WINDOW_BACKGROUND_COLOR);
    await w.loadURL('data:text/html,<html></html>');

    view = new BrowserView();
    view.setBounds(display.bounds);
    w.setBrowserView(view);
    w.setBackgroundColor(VIEW_BACKGROUND_COLOR);
    await view.webContents.loadURL('data:text/html,hello there');

    const screenCapture = new ScreenCapture(display);
    await screenCapture.expectColorAtCenterMatches(VIEW_BACKGROUND_COLOR);
  });
});
