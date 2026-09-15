import { BaseWindow, View } from 'electron/main';

import { expect } from 'chai';

import { closeWindow } from './lib/window-helpers';

// Layout API methods are added by the feature in this PR but the public
// docs/api/view.md entries (and the regenerated electron.d.ts) are part of
// @nikwen's separate documentation effort. Declare them here so the tests
// type-check until the doc-generated types catch up.
declare module 'electron/main' {
  interface View {
    setLayout(options: object | null): void;
    setLayoutFlex(spec: object | null): void;
    setLayoutMargins(margins: { top?: number; left?: number; bottom?: number; right?: number }): void;
    setBoxFlex(child: View, weight: number, useMinSize?: boolean): void;
    layout(): void;
    invalidateLayout(): void;
    setPreferredSize(size: { width: number; height: number }): void;
    getPreferredSize(): { width: number; height: number };
    setUseDefaultFillLayout(use: boolean): void;
    getLayoutManagerType(): string;
    getLayoutFlex(): object | null;
    getOrientation(): string;
    getMainAxisAlignment(): string;
    getCrossAxisAlignment(): string;
    getBetweenChildSpacing(): number;
    getInsideBorderInsets(): { top: number; left: number; bottom: number; right: number };
  }
}

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

  describe('view.setLayout', () => {
    it('accepts type:"fill" and stretches a child to host bounds', () => {
      w = new BaseWindow({ show: false });
      const root = new View();
      w.setContentView(root);
      const child = new View();
      root.addChildView(child);
      root.setLayout({ type: 'fill' } as any);
      root.setBounds({ x: 0, y: 0, width: 400, height: 300 });
      (root as any).layout();
      expect(child.getBounds()).to.deep.equal({ x: 0, y: 0, width: 400, height: 300 });
    });

    it('accepts type:"box" horizontal and lays out children side by side', () => {
      w = new BaseWindow({ show: false });
      const root = new View();
      w.setContentView(root);
      const a = new View();
      const b = new View();
      // Layout managers honour setPreferredSize, not setBounds.
      a.setPreferredSize({ width: 100, height: 50 });
      b.setPreferredSize({ width: 150, height: 50 });
      root.addChildView(a);
      root.addChildView(b);
      root.setLayout({ type: 'box', orientation: 'horizontal', betweenChildSpacing: 10 } as any);
      root.setBounds({ x: 0, y: 0, width: 400, height: 100 });
      (root as any).layout();
      expect(a.getBounds().x).to.equal(0);
      expect(a.getBounds().width).to.equal(100);
      // 100 (a width) + 10 spacing = 110
      expect(b.getBounds().x).to.equal(110);
      expect(b.getBounds().width).to.equal(150);
    });

    it('accepts type:"flex" (default) for backward compatibility', () => {
      w = new BaseWindow({ show: false });
      const root = new View();
      w.setContentView(root);
      // Old shape (no `type` field) still produces a flex layout.
      root.setLayout({ orientation: 'vertical', crossAxisAlignment: 'stretch' } as any);
      expect((root as any).getLayoutManagerType()).to.equal('flex');
    });

    it('setLayout(null) removes the layout manager', () => {
      const root = new View();
      root.setLayout({ type: 'fill' } as any);
      expect((root as any).getLayoutManagerType()).to.equal('fill');
      root.setLayout(null as any);
      expect((root as any).getLayoutManagerType()).to.equal('');
    });

    it('throws on non-object, non-null argument', () => {
      const root = new View();
      expect(() => root.setLayout(42 as any)).to.throw();
    });

    it('throws on an unknown type field', () => {
      const v = new View();
      expect(() => v.setLayout({ type: 'fluss' } as any))
        .to.throw(/Unknown layout type/);
    });

    it('still accepts the legacy no-type shape as flex', () => {
      // setLayout({orientation, crossAxisAlignment, ...}) from PR #35658
      // was without a `type` field. Must keep working for back-compat.
      const v = new View();
      expect(() => v.setLayout({ orientation: 'vertical' } as any)).to.not.throw();
      expect((v as any).getLayoutManagerType()).to.equal('flex');
    });

    it('setLayout(null) clears a JS layout manager', () => {
      const v = new View();
      v.setLayout({
        calculateProposedLayout: () => ({ size: { width: 0, height: 0 }, layouts: [] })
      } as any);
      expect((v as any).getLayoutManagerType()).to.equal('js');
      v.setLayout(null as any);
      expect((v as any).getLayoutManagerType()).to.equal('');
    });

    it('JS calculateProposedLayout callback is invoked', () => {
      w = new BaseWindow({ show: false });
      const root = new View();
      w.setContentView(root);
      const child = new View();
      root.addChildView(child);
      let invocations = 0;
      root.setLayout({
        calculateProposedLayout: (sb: any) => {
          invocations++;
          return {
            size: { width: sb.width || 0, height: sb.height || 0 },
            layouts: [{
              view: child,
              bounds: { x: 0, y: 0, width: (sb.width || 0) / 2, height: sb.height || 0 }
            }]
          };
        }
      } as any);
      root.setBounds({ x: 0, y: 0, width: 400, height: 200 });
      (root as any).layout();
      expect(invocations).to.be.greaterThan(0);
      expect(child.getBounds().width).to.equal(200);
    });

    it('does not crash when the JS layout callback throws', () => {
      w = new BaseWindow({ show: false });
      const root = new View();
      w.setContentView(root);
      const child = new View();
      root.addChildView(child);
      root.setLayout({
        calculateProposedLayout: () => {
          throw new Error('boom');
        }
      } as any);
      // gin's V8FunctionInvoker absorbs the JS exception (it returns a
      // default-constructed ProposedLayout) so the C++ side stays sane;
      // the pending v8 exception is delivered to Node on next reentry.
      expect(() => {
        root.setBounds({ x: 0, y: 0, width: 100, height: 100 });
        (root as any).layout();
      }).to.not.throw();
    });

    it('blocks re-entrant setLayout from within JS callback', () => {
      w = new BaseWindow({ show: false });
      const root = new View();
      w.setContentView(root);
      let reentryError: Error | null = null;
      root.setLayout({
        calculateProposedLayout: (sb: any) => {
          try {
            root.setLayout({ type: 'fill' } as any);
          } catch (e) {
            reentryError = e as Error;
          }
          return { size: { width: sb.width || 0, height: sb.height || 0 }, layouts: [] };
        }
      } as any);
      root.setBounds({ x: 0, y: 0, width: 100, height: 100 });
      (root as any).layout();
      expect(reentryError).to.not.equal(null);
      expect(reentryError!.message).to.match(/Cannot call setLayout/);
    });
  });

  describe('view.setLayoutFlex', () => {
    it('flex unbounded grows the child to fill', () => {
      w = new BaseWindow({ show: false });
      const root = new View();
      w.setContentView(root);
      const child = new View();
      root.addChildView(child);
      root.setLayout({ type: 'flex', orientation: 'vertical', crossAxisAlignment: 'stretch' } as any);
      (child as any).setLayoutFlex({ minimum: 'scale-to-minimum', maximum: 'unbounded', weight: 1 });
      root.setBounds({ x: 0, y: 0, width: 500, height: 200 });
      (root as any).layout();
      expect(child.getBounds().width).to.equal(500);
      expect(child.getBounds().height).to.equal(200);
    });

    it('setLayoutFlex(null) clears the flex behavior', () => {
      const child = new View();
      (child as any).setLayoutFlex({ minimum: 'scale-to-minimum', maximum: 'unbounded', weight: 1 });
      (child as any).setLayoutFlex(null);
      expect((child as any).getLayoutFlex()).to.equal(null);
    });

    it('throws on invalid spec', () => {
      const child = new View();
      expect(() => (child as any).setLayoutFlex(42)).to.throw();
    });

    it('getLayoutFlex preserves minimum and maximum rules', () => {
      const child = new View();
      (child as any).setLayoutFlex({ minimum: 'scale-to-zero', maximum: 'unbounded', weight: 3, order: 2 });
      const spec = (child as any).getLayoutFlex();
      expect(spec).to.not.equal(null);
      expect(spec.weight).to.equal(3);
      expect(spec.order).to.equal(2);
      expect(spec.minimum).to.equal('scale-to-zero');
      expect(spec.maximum).to.equal('unbounded');
    });

    it('getLayoutFlex omits min/max after they are cleared', () => {
      const child = new View();
      (child as any).setLayoutFlex({ minimum: 'scale-to-minimum', maximum: 'unbounded', weight: 1 });
      (child as any).setLayoutFlex(null);
      expect((child as any).getLayoutFlex()).to.equal(null);
      // Re-setting without min/max should not leak previous cached values.
      (child as any).setLayoutFlex({ weight: 5 });
      const spec = (child as any).getLayoutFlex();
      expect(spec.weight).to.equal(5);
      expect(spec.minimum).to.equal(undefined);
      expect(spec.maximum).to.equal(undefined);
    });
  });

  describe('view.setUseDefaultFillLayout', () => {
    it('does not throw and stretches a single child to fill the parent', () => {
      w = new BaseWindow({ show: false });
      const root = new View();
      w.setContentView(root);
      const child = new View();
      root.addChildView(child);
      expect(() => (root as any).setUseDefaultFillLayout(true)).to.not.throw();
      root.setBounds({ x: 0, y: 0, width: 300, height: 150 });
      (root as any).layout();
      const bounds = child.getBounds();
      expect(bounds.width).to.equal(300);
      expect(bounds.height).to.equal(150);
    });

    it('accepts toggling false without affecting layout manager type', () => {
      const root = new View();
      (root as any).setUseDefaultFillLayout(true);
      (root as any).setUseDefaultFillLayout(false);
      // setUseDefaultFillLayout doesn't go through our SetLayout(), so the
      // tracked layout_type_ stays at its previous value (none in this test).
      expect((root as any).getLayoutManagerType()).to.equal('');
    });
  });

  describe('view.setLayoutMargins', () => {
    it('applies margins around a flexed child', () => {
      w = new BaseWindow({ show: false });
      const root = new View();
      w.setContentView(root);
      const child = new View();
      root.addChildView(child);
      root.setLayout({ type: 'flex', orientation: 'vertical', crossAxisAlignment: 'stretch' } as any);
      (child as any).setLayoutFlex({ minimum: 'scale-to-minimum', maximum: 'unbounded', weight: 1 });
      (child as any).setLayoutMargins({ top: 10, left: 5, bottom: 10, right: 5 });
      root.setBounds({ x: 0, y: 0, width: 200, height: 200 });
      (root as any).layout();
      const bounds = child.getBounds();
      expect(bounds.x).to.equal(5);
      expect(bounds.y).to.equal(10);
      expect(bounds.width).to.equal(190);
      expect(bounds.height).to.equal(180);
    });
  });

  describe('view.getLayoutManagerType', () => {
    it('returns empty string when no layout is set', () => {
      const v = new View();
      expect((v as any).getLayoutManagerType()).to.equal('');
    });

    it('returns "fill" for fill layout', () => {
      const v = new View();
      v.setLayout({ type: 'fill' } as any);
      expect((v as any).getLayoutManagerType()).to.equal('fill');
    });

    it('returns "box" for box layout', () => {
      const v = new View();
      v.setLayout({ type: 'box' } as any);
      expect((v as any).getLayoutManagerType()).to.equal('box');
    });

    it('returns "flex" for flex layout', () => {
      const v = new View();
      v.setLayout({ type: 'flex' } as any);
      expect((v as any).getLayoutManagerType()).to.equal('flex');
    });

    it('returns "js" for custom JS layout', () => {
      const v = new View();
      v.setLayout({
        calculateProposedLayout: () => ({ size: { width: 0, height: 0 }, layouts: [] })
      } as any);
      expect((v as any).getLayoutManagerType()).to.equal('js');
    });
  });

  describe('view.setPreferredSize|getPreferredSize', () => {
    it('round-trips a preferred size', () => {
      const v = new View();
      (v as any).setPreferredSize({ width: 250, height: 175 });
      expect((v as any).getPreferredSize()).to.deep.equal({ width: 250, height: 175 });
    });
  });

  describe('view.setBoxFlex', () => {
    it('throws when called outside a box layout', () => {
      w = new BaseWindow({ show: false });
      const root = new View();
      w.setContentView(root);
      const child = new View();
      root.addChildView(child);
      root.setLayout({ type: 'flex' } as any);
      expect(() => (root as any).setBoxFlex(child, 1)).to.throw(/box layout/);
    });

    it('throws for a non-child view', () => {
      const root = new View();
      const stranger = new View();
      root.setLayout({ type: 'box' } as any);
      expect(() => (root as any).setBoxFlex(stranger, 1)).to.throw(/direct child/);
    });

    it('does not throw when applied to a real child', () => {
      const root = new View();
      const child = new View();
      root.addChildView(child);
      root.setLayout({ type: 'box', orientation: 'horizontal' } as any);
      expect(() => (root as any).setBoxFlex(child, 1, true)).to.not.throw();
    });

    it('accepts weight 0 (child takes only its preferred size)', () => {
      const root = new View();
      const child = new View();
      root.addChildView(child);
      root.setLayout({ type: 'box', orientation: 'horizontal' } as any);
      expect(() => (root as any).setBoxFlex(child, 0)).to.not.throw();
    });
  });

  describe('view.layout|invalidateLayout', () => {
    it('layout() does not throw on a view without a layout manager', () => {
      const v = new View();
      expect(() => (v as any).layout()).to.not.throw();
    });

    it('invalidateLayout() does not throw on a view without a layout manager', () => {
      const v = new View();
      expect(() => (v as any).invalidateLayout()).to.not.throw();
    });
  });

  describe('view box-layout getters', () => {
    it('getOrientation returns the configured value for box layout', () => {
      const v = new View();
      v.setLayout({ type: 'box', orientation: 'vertical' } as any);
      expect((v as any).getOrientation()).to.equal('vertical');
      v.setLayout({ type: 'box', orientation: 'horizontal' } as any);
      expect((v as any).getOrientation()).to.equal('horizontal');
    });

    it('getOrientation returns "" when no box/flex layout is set', () => {
      const v = new View();
      expect((v as any).getOrientation()).to.equal('');
      v.setLayout({ type: 'fill' } as any);
      expect((v as any).getOrientation()).to.equal('');
    });

    it('getBetweenChildSpacing reflects the option set on the layout', () => {
      const v = new View();
      v.setLayout({ type: 'box', betweenChildSpacing: 17 } as any);
      expect((v as any).getBetweenChildSpacing()).to.equal(17);
    });

    it('getInsideBorderInsets returns zero insets by default', () => {
      const v = new View();
      v.setLayout({ type: 'box' } as any);
      const insets = (v as any).getInsideBorderInsets();
      expect(insets.top + insets.left + insets.bottom + insets.right).to.equal(0);
    });

    it('getMainAxisAlignment works for flex layout', () => {
      const v = new View();
      v.setLayout({ type: 'flex', mainAxisAlignment: 'center' } as any);
      expect((v as any).getMainAxisAlignment()).to.equal('center');
    });

    it('getCrossAxisAlignment works for flex layout', () => {
      // Always set crossAxisAlignment explicitly when probing FlexLayout — the
      // Chromium accessor dereferences an internal optional, undefined behavior
      // if SetCrossAxisAlignment was never called. See flex_layout.h:101-103.
      const v = new View();
      v.setLayout({ type: 'flex', crossAxisAlignment: 'stretch' } as any);
      expect((v as any).getCrossAxisAlignment()).to.equal('stretch');
    });
  });
});
