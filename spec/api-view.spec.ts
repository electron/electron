import { nativeImage } from 'electron/common';
import { BaseWindow, ImageView, View, WebContentsView } from 'electron/main';

import { afterEach, describe, expect, it } from 'vitest';

import * as fs from 'node:fs';

import { closeWindow } from './lib/window-helpers.ts';

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
      expect(order).toEqual(expected);
    });

    it('does not run pending microtasks re-entrantly (from a promise continuation)', async () => {
      const order = await Promise.resolve().then(probe);
      expect(order).toEqual(expected);
    });

    it('does not run pending microtasks re-entrantly (from a libuv callback)', async () => {
      const order = await new Promise<string[]>((resolve) => fs.stat(import.meta.filename, () => resolve(probe())));
      expect(order).toEqual(expected);
    });
  });

  it('can be used as content view', () => {
    w = new BaseWindow({ show: false });
    const v = new View();
    w.setContentView(v);
    expect(w.contentView).toBe(v);
  });

  it('will throw when added as a child to itself', () => {
    w = new BaseWindow({ show: false });
    expect(() => {
      w.contentView.addChildView(w.contentView);
    }).toThrow('A view cannot be added as its own child');
  });

  it('does not crash when attempting to add a child multiple times', () => {
    w = new BaseWindow({ show: false });
    const cv = new View();
    w.setContentView(cv);

    const v = new View();
    w.contentView.addChildView(v);
    w.contentView.addChildView(v);
    w.contentView.addChildView(v);

    expect(w.contentView.children).toHaveLength(1);
  });

  it('can be added as a child of another View', async () => {
    const w = new BaseWindow();
    const v1 = new View();
    const v2 = new View();

    v1.addChildView(v2);
    w.contentView.addChildView(v1);

    expect(w.contentView.children).toEqual([v1]);
    expect(v1.children).toEqual([v2]);
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

    expect(w.contentView.children).toEqual([v1, v2, v3]);

    w.contentView.addChildView(v1);
    w.contentView.addChildView(v2);
    expect(w.contentView.children).toEqual([v3, v1, v2]);
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
      expect(v.getVisible()).toBe(true);
    });

    it('can be set to not visible', () => {
      const v = new View();
      v.setVisible(false);
      expect(v.getVisible()).toBe(false);
    });
  });

  describe('view.getBounds|setBounds', () => {
    it('defaults to 0,0,0,0', () => {
      const v = new View();
      expect(v.getBounds()).toEqual({ x: 0, y: 0, width: 0, height: 0 });
    });

    it('can be set and retrieved', () => {
      const v = new View();
      v.setBounds({ x: 10, y: 20, width: 300, height: 400 });
      expect(v.getBounds()).toEqual({ x: 10, y: 20, width: 300, height: 400 });
    });

    it('emits bounds-changed when bounds mutate', () => {
      const v = new View();
      let called = 0;
      v.once('bounds-changed', () => {
        called++;
      });
      v.setBounds({ x: 5, y: 6, width: 7, height: 8 });
      expect(called).toBe(1);
    });

    it('allows zero-size bounds', () => {
      const v = new View();
      v.setBounds({ x: 1, y: 2, width: 0, height: 0 });
      expect(v.getBounds()).toEqual({ x: 1, y: 2, width: 0, height: 0 });
    });

    it('allows negative coordinates', () => {
      const v = new View();
      v.setBounds({ x: -10, y: -20, width: 100, height: 50 });
      expect(v.getBounds()).toEqual({ x: -10, y: -20, width: 100, height: 50 });
    });

    it('child bounds remain relative after parent moves', () => {
      const parent = new View();
      const child = new View();
      parent.addChildView(child);
      child.setBounds({ x: 10, y: 15, width: 25, height: 30 });
      parent.setBounds({ x: 50, y: 60, width: 500, height: 600 });
      expect(child.getBounds()).toEqual({ x: 10, y: 15, width: 25, height: 30 });
    });

    it('can set bounds with animation', () =>
      new Promise<void>((resolve) => {
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
          expect(v.getBounds()).toEqual({ x: 0, y: 0, width: 100, height: 100 });
          resolve();
        }, 350);
      }));
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
      }).not.toThrow();
    });
  });

  describe('constructors', () => {
    it('throw when called without new', () => {
      expect(() => (View as any)()).toThrow('Requires constructor call');
      expect(() => (ImageView as any)()).toThrow('Requires constructor call');
      expect(() => (WebContentsView as any)()).toThrow('Requires constructor call');
    });

    it('keep the prototype of JavaScript subclasses', () => {
      class MyView extends View {
        kind() {
          return 'my-view';
        }
      }
      class MyImageView extends ImageView {}
      class MyWebContentsView extends WebContentsView {}

      const v = new MyView();
      const iv = new MyImageView();
      const wcv = new MyWebContentsView();
      try {
        expect(v).toBeInstanceOf(MyView);
        expect(v).toBeInstanceOf(View);
        expect(v.kind()).toBe('my-view');
        expect(iv).toBeInstanceOf(MyImageView);
        expect(iv).toBeInstanceOf(ImageView);
        expect(iv).toBeInstanceOf(View);
        expect(wcv).toBeInstanceOf(MyWebContentsView);
        expect(wcv).toBeInstanceOf(WebContentsView);

        const parent = new View();
        parent.addChildView(v);
        parent.addChildView(iv);
        parent.addChildView(wcv);
        expect(parent.children).toEqual([v, iv, wcv]);
        expect(parent.children[0]).toBe(v);

        let emitted = 0;
        v.on('bounds-changed', () => emitted++);
        v.setBounds({ x: 0, y: 0, width: 10, height: 10 });
        expect(emitted).toBe(1);
        expect(v.getBounds()).toEqual({ x: 0, y: 0, width: 10, height: 10 });
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
        expect(parent.children).toEqual([a, b]);
        expect(parent.children[0]).toBe(a);
        expect(parent.children[1]).toBe(b);

        parent.removeChildView(b);
        expect(parent.children).toEqual([a]);
        expect(parent.children[0]).toBe(a);
      } finally {
        a.webContents.destroy();
      }
    });

    it('can be called on subclass instances', () => {
      const iv = new ImageView();
      iv.setBounds({ x: 1, y: 2, width: 3, height: 4 });
      expect(View.prototype.getBounds.call(iv)).toEqual({ x: 1, y: 2, width: 3, height: 4 });
    });

    it('throw when called on an object of another type', () => {
      const v = new View();
      expect(() => ImageView.prototype.setImage.call(v, nativeImage.createEmpty())).toThrow('Illegal invocation');
      expect(() => (WebContentsView.prototype.setBorderRadius as any).call(v, 1)).toThrow('Illegal invocation');
      expect(() => View.prototype.getBounds.call(nativeImage.createEmpty())).toThrow('Illegal invocation');
    });
  });
});
