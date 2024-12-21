import { BaseWindow, View } from 'electron/main';

import { expect } from 'chai';

import { closeWindow } from './lib/window-helpers';

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

  it('does not crash when attempting to add a child multiple times', () => {
    w = new BaseWindow({ show: false });
    const cv = new View();
    w.setContentView(cv);

    const v = new View();
    w.contentView.addChildView(v);
    w.contentView.addChildView(v);
    w.contentView.addChildView(v);

    expect(w.contentView.children).to.have.lengthOf(1);
  });

  it('can be added as a child of another View', async () => {
    const w = new BaseWindow();
    const v1 = new View();
    const v2 = new View();

    v1.addChildView(v2);
    w.contentView.addChildView(v1);

    expect(w.contentView.children).to.deep.equal([v1]);
    expect(v1.children).to.deep.equal([v2]);
  });

  it('correctly reorders children', () => {
    w = new BaseWindow({ show: false });
    const cv = new View();
    w.setContentView(cv);

    const v1 = new View();
    const v2 = new View();
    const v3 = new View();
    w.contentView.addChildView(v1);
    w.contentView.addChildView(v2);
    w.contentView.addChildView(v3);

    expect(w.contentView.children).to.deep.equal([v1, v2, v3]);

    w.contentView.addChildView(v1);
    w.contentView.addChildView(v2);
    expect(w.contentView.children).to.deep.equal([v3, v1, v2]);
  });

  it('allows setting various border radius values', () => {
    w = new BaseWindow({ show: false });
    const v = new View();
    w.setContentView(v);
    v.setBorderRadius(10);
    v.setBorderRadius(0);
    v.setBorderRadius(-10);
    v.setBorderRadius(9999999);
    v.setBorderRadius(-9999999);
  });
});
