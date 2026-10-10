import { nativeImage } from 'electron/common';
import { BaseWindow, ImageView, View, WebContentsView } from 'electron/main';

import { expect } from 'chai';

import * as fs from 'node:fs';

import { closeWindow, closeAllWindows } from './lib/window-helpers.ts';

describe('View', () => {
  let w: BaseWindow;
  afterEach(async () => {
    await closeWindow(w as any);
    w = null as unknown as BaseWindow;
  });

  describe('setBounds', () => {
    // setBounds() emits 'bounds-changed' synchronously from native code. A
    // promise continuation queued before the call must not run inside it,
    // whichever way the calling JS was entered.
    const probe = () => {
      const v = new View();
      const order: string[] = [];
      v.once('bounds-changed', () => order.push('bounds-changed'));
      Promise.resolve().then(() => order.push('microtask'));
      v.setBounds({ x: 0, y: 0, width: 7, height: 7 });
      order.push('returned');
      return new Promise<string[]>((resolve) => setImmediate(() => resolve(order)));
    };
    const expected = ['bounds-changed', 'returned', 'microtask'];

    it('does not run pending microtasks re-entrantly (from a timer)', async () => {
      const order = await new Promise<string[]>((resolve) => setTimeout(() => resolve(probe())));
      expect(order).to.deep.equal(expected);
    });

    it('does not run pending microtasks re-entrantly (from a promise continuation)', async () => {
      const order = await Promise.resolve().then(probe);
      expect(order).to.deep.equal(expected);
    });

    it('does not run pending microtasks re-entrantly (from a libuv callback)', async () => {
      const order = await new Promise<string[]>((resolve) => fs.stat(import.meta.filename, () => resolve(probe())));
      expect(order).to.deep.equal(expected);
    });
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
      v.once('bounds-changed', () => {
        called++;
      });
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
      v.setBounds(
        { x: 0, y: 0, width: 100, height: 100 },
        {
          animate: {
            duration: 300
          }
        }
      );
      setTimeout(() => {
        expect(v.getBounds()).to.deep.equal({ x: 0, y: 0, width: 100, height: 100 });
        done();
      }, 350);
    });
  });

  describe('view.setBackgroundBlur', () => {
    it('can be set to various values', () => {
      w = new BaseWindow({ show: false });
      const v = new View();
      w.setContentView(v);
      v.setBackgroundBlur(0);
      v.setBackgroundBlur(10);
      v.setBackgroundBlur(-10);
      v.setBackgroundBlur(100);
      v.setBackgroundBlur(-100);
    });

    it('does not throw when set before being added to a window', () => {
      const v = new View();
      expect(() => {
        v.setBackgroundBlur(10);
      }).to.not.throw();
    });
  });

  describe('constructors', () => {
    it('throw when called without new', () => {
      expect(() => (View as any)()).to.throw('Requires constructor call');
      expect(() => (ImageView as any)()).to.throw('Requires constructor call');
      expect(() => (WebContentsView as any)()).to.throw('Requires constructor call');
    });

    it('keep the prototype of JavaScript subclasses', () => {
      class MyView extends View {
        kind() {
          return 'my-view';
        }
      }
      class MyImageView extends ImageView { }
      class MyWebContentsView extends WebContentsView { }

      const v = new MyView();
      const iv = new MyImageView();
      const wcv = new MyWebContentsView();
      try {
        expect(v).to.be.an.instanceOf(MyView).and.an.instanceOf(View);
        expect(v.kind()).to.equal('my-view');
        expect(iv).to.be.an.instanceOf(MyImageView).and.an.instanceOf(ImageView).and.an.instanceOf(View);
        expect(wcv).to.be.an.instanceOf(MyWebContentsView).and.an.instanceOf(WebContentsView);

        const parent = new View();
        parent.addChildView(v);
        parent.addChildView(iv);
        parent.addChildView(wcv);
        expect(parent.children).to.deep.equal([v, iv, wcv]);
        expect(parent.children[0]).to.equal(v);

        let emitted = 0;
        v.on('bounds-changed', () => emitted++);
        v.setBounds({ x: 0, y: 0, width: 10, height: 10 });
        expect(emitted).to.equal(1);
        expect(v.getBounds()).to.deep.equal({ x: 0, y: 0, width: 10, height: 10 });
      } finally {
        wcv.webContents.destroy();
      }
    });
  });

  describe('methods', () => {
    it('treat two WebContentsViews that share a webContents as separate children', () => {
      const a = new WebContentsView();
      const b = new WebContentsView({ webContents: a.webContents });
      try {
        const parent = new View();
        parent.addChildView(a);
        parent.addChildView(b);
        expect(parent.children).to.deep.equal([a, b]);
        expect(parent.children[0]).to.equal(a);
        expect(parent.children[1]).to.equal(b);

        parent.removeChildView(b);
        expect(parent.children).to.deep.equal([a]);
        expect(parent.children[0]).to.equal(a);
      } finally {
        a.webContents.destroy();
      }
    });

    it('can be called on subclass instances', () => {
      const iv = new ImageView();
      iv.setBounds({ x: 1, y: 2, width: 3, height: 4 });
      expect(View.prototype.getBounds.call(iv)).to.deep.equal({ x: 1, y: 2, width: 3, height: 4 });
    });

    it('throw when called on an object of another type', () => {
      const v = new View();
      expect(() => ImageView.prototype.setImage.call(v, nativeImage.createEmpty())).to.throw('Illegal invocation');
      expect(() => (WebContentsView.prototype.setBorderRadius as any).call(v, 1)).to.throw('Illegal invocation');
      expect(() => View.prototype.getBounds.call(nativeImage.createEmpty())).to.throw('Illegal invocation');
    });
  });

  describe('setInteractive', () => {
    afterEach(closeAllWindows);

    it('does not throw when toggled on a view', () => {
      w = new BaseWindow({ show: false });
      const v = new View();
      expect(() => v.setInteractive(false)).to.not.throw();
      expect(() => v.setInteractive(true)).to.not.throw();
      w.setContentView(v);
      expect(() => v.setInteractive(false)).to.not.throw();
      expect(() => v.setInteractive(true)).to.not.throw();
    });

    it('correctly records state when toggled', () => {
      const v = new View();
      expect(v.getInteractive()).to.be.true();
      v.setInteractive(false);
      expect(v.getInteractive()).to.be.false();
      v.setInteractive(true);
      expect(v.getInteractive()).to.be.true();
    });
  });
});
