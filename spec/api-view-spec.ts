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

  describe('view.getVisible|setVisible', () => {
    it('is visible by default', () => {
      const v = new View();
      expect(v.getVisible()).to.be.true();
    });

    it('can be set to not visible', () => {
      const v = new View();
      v.setVisible(false);
      expect(v.getVisible()).to.be.false();
    });
  });

  describe('view.getBounds|setBounds', () => {
    it('defaults to 0,0,0,0', () => {
      const v = new View();
      expect(v.getBounds()).to.deep.equal({ x: 0, y: 0, width: 0, height: 0 });
    });

    it('can be set and retrieved', () => {
      const v = new View();
      v.setBounds({ x: 10, y: 20, width: 300, height: 400 });
      expect(v.getBounds()).to.deep.equal({ x: 10, y: 20, width: 300, height: 400 });
    });

    it('emits bounds-changed when bounds mutate', () => {
      const v = new View();
      let called = 0;
      v.once('bounds-changed', () => { called++; });
      v.setBounds({ x: 5, y: 6, width: 7, height: 8 });
      expect(called).to.equal(1);
    });

    it('allows zero-size bounds', () => {
      const v = new View();
      v.setBounds({ x: 1, y: 2, width: 0, height: 0 });
      expect(v.getBounds()).to.deep.equal({ x: 1, y: 2, width: 0, height: 0 });
    });

    it('allows negative coordinates', () => {
      const v = new View();
      v.setBounds({ x: -10, y: -20, width: 100, height: 50 });
      expect(v.getBounds()).to.deep.equal({ x: -10, y: -20, width: 100, height: 50 });
    });

    it('child bounds remain relative after parent moves', () => {
      const parent = new View();
      const child = new View();
      parent.addChildView(child);
      child.setBounds({ x: 10, y: 15, width: 25, height: 30 });
      parent.setBounds({ x: 50, y: 60, width: 500, height: 600 });
      expect(child.getBounds()).to.deep.equal({ x: 10, y: 15, width: 25, height: 30 });
    });

    it('can set bounds with animation', (done) => {
      const v = new View();
      v.setBounds({ x: 0, y: 0, width: 100, height: 100 }, {
        animate: {
          duration: 300
        }
      });
      setTimeout(() => {
        expect(v.getBounds()).to.deep.equal({ x: 0, y: 0, width: 100, height: 100 });
        done();
      }, 350);
    });
  });
});
