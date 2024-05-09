import { expect } from 'chai';
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
    const v = new View();
    w.setContentView(v);
    expect(w.contentView).to.equal(v);
  });

  it('will throw when added as a child to itself', () => {
    w = new BaseWindow({ show: false });
    expect(() => {
      w.contentView.addChildView(w.contentView);
    }).to.throw('A view cannot be added as its own child');
  });
});
