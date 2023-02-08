import { closeWindow } from './lib/window-helpers';
import { BaseWindow, View } from 'electron/main';

describe('View', () => {
  let w: BaseWindow;
  afterEach(async () => {
    await closeWindow(w as any);
    w = null as unknown as BaseWindow;
  });

  it('can be used as content view', () => {
    w = new BaseWindow({ show: false });
    w.setContentView(new View());
  });
});
