import { closeWindow } from './lib/window-helpers';
import { BaseWindow } from 'electron/main';

describe('View', () => {
  let w: BaseWindow;
  afterEach(async () => {
    await closeWindow(w as any);
    w = null as unknown as BaseWindow;
  });
});
