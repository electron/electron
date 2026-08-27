import { BaseWindow, Menu, View, screen } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';

import { ifdescribe, isWayland } from './lib/spec-helpers';
import { closeWindow, closeAllWindows } from './lib/window-helpers';

// Is the display's scale factor possibly causing rounding of pixel coordinate
// values?
const isScaleFactorRounding = () => {
  const { scaleFactor } = screen.getPrimaryDisplay();
  // Return true if scale factor is non-integer value
  if (Math.round(scaleFactor) !== scaleFactor) return true;
  // Return true if scale factor is odd number above 2
  return scaleFactor > 2 && scaleFactor % 2 === 1;
};

const expectBoundsEqual = (actual: any, expected: any) => {
  if (!isScaleFactorRounding()) {
    expect(actual).to.deep.equal(expected);
  } else if (Array.isArray(actual)) {
    expect(actual[0]).to.be.closeTo(expected[0], 1);
    expect(actual[1]).to.be.closeTo(expected[1], 1);
  } else {
    expect(actual.x).to.be.closeTo(expected.x, 1);
    expect(actual.y).to.be.closeTo(expected.y, 1);
    expect(actual.width).to.be.closeTo(expected.width, 1);
    expect(actual.height).to.be.closeTo(expected.height, 1);
  }
};

describe('BaseWindow module', () => {
  it('sets the correct class name on the prototype', () => {
    expect(BaseWindow.prototype.constructor.name).to.equal('BaseWindow');
  });

  describe('BaseWindow constructor', () => {
    afterEach(closeAllWindows);

    it('options argument is optional', () => {
      expect(() => {
        const w = new BaseWindow();
        w.destroy();
      }).to.not.throw();
    });

    it('creates a hidden window when show is false', () => {
      const w = new BaseWindow({ show: false });
      expect(w.isVisible()).to.be.false('is visible');
    });

    it('honors the width and height options', () => {
      const w = new BaseWindow({ show: false, width: 400, height: 300 });
      expectBoundsEqual(w.getSize(), [400, 300]);
    });

    it('reports requested bounds when created with explicit x/y', () => {
      const requested = { x: 120, y: 140, width: 420, height: 320 };
      const w = new BaseWindow({ show: false, ...requested });
      expectBoundsEqual(w.getBounds(), requested);
    });
  });

  describe('BaseWindow.close()', () => {
    afterEach(closeAllWindows);

    it("emits 'close' and then 'closed'", async () => {
      const w = new BaseWindow({ show: false });
      const events: string[] = [];
      w.on('close', () => events.push('close'));
      const closed = once(w, 'closed');
      w.on('closed', () => events.push('closed'));
      w.close();
      await closed;
      expect(events).to.deep.equal(['close', 'closed']);
    });

    it('cancels the close when event.preventDefault() is called', () => {
      const w = new BaseWindow({ show: false });
      w.once('close', (e) => {
        e.preventDefault();
      });
      w.close();
      expect(w.isDestroyed()).to.be.false('window is destroyed');
    });
  });

  describe('BaseWindow.destroy()', () => {
    afterEach(closeAllWindows);

    it("emits 'closed' but not 'close'", async () => {
      const w = new BaseWindow({ show: false });
      w.on('close', () => {
        expect.fail('close event should not be emitted');
      });
      const closed = once(w, 'closed');
      w.destroy();
      await closed;
      expect(w.isDestroyed()).to.be.true('window is not destroyed');
    });
  });

  describe('BaseWindow.getAllWindows()', () => {
    afterEach(closeAllWindows);

    it('returns all created windows', () => {
      const w1 = new BaseWindow({ show: false });
      const w2 = new BaseWindow({ show: false });
      const windows = BaseWindow.getAllWindows();
      expect(windows).to.include(w1);
      expect(windows).to.include(w2);
    });
  });

  describe('BaseWindow.fromId(id)', () => {
    afterEach(closeAllWindows);

    it('returns the window with the given id', () => {
      const w = new BaseWindow({ show: false });
      expect(BaseWindow.fromId(w.id)).to.equal(w);
    });

    it('returns null for a nonexistent id', () => {
      expect(BaseWindow.fromId(314159)).to.be.null('window');
    });
  });

  describe('win.id', () => {
    afterEach(closeAllWindows);

    it('is unique for each window', () => {
      const w1 = new BaseWindow({ show: false });
      const w2 = new BaseWindow({ show: false });
      expect(w1.id).to.be.a('number');
      expect(w2.id).to.not.equal(w1.id);
    });
  });

  describe('content view', () => {
    afterEach(closeAllWindows);

    it('window has a content view by default', () => {
      const w = new BaseWindow({ show: false });
      expect(w.contentView).to.be.an.instanceOf(View);
    });

    it('BaseWindow.setContentView(view) replaces the content view', () => {
      const w = new BaseWindow({ show: false });
      const v = new View();
      w.setContentView(v);
      expect(w.contentView).to.equal(v);
    });

    it('BaseWindow.getContentView() returns the current content view', () => {
      const w = new BaseWindow({ show: false });
      expect(w.getContentView()).to.equal(w.contentView);
      const v = new View();
      w.setContentView(v);
      expect(w.getContentView()).to.equal(v);
    });
  });

  describe('BaseWindow.isModal()', () => {
    afterEach(closeAllWindows);

    it('returns false for a normal window', () => {
      const w = new BaseWindow({ show: false });
      expect(w.isModal()).to.be.false('isModal');
    });

    it('returns true for a modal child window', () => {
      const parent = new BaseWindow({ show: false });
      const child = new BaseWindow({ show: false, parent, modal: true });
      expect(child.isModal()).to.be.true('isModal');
    });
  });

  describe('parent and child windows', () => {
    afterEach(closeAllWindows);

    it('parent option sets the parent window', () => {
      const parent = new BaseWindow({ show: false });
      const child = new BaseWindow({ show: false, parent });
      expect(child.getParentWindow()).to.equal(parent);
      expect(parent.getParentWindow()).to.be.null('parent window');
    });

    it('getChildWindows() returns the child windows', () => {
      const parent = new BaseWindow({ show: false });
      expect(parent.getChildWindows()).to.have.lengthOf(0);
      const child = new BaseWindow({ show: false, parent });
      expect(parent.getChildWindows()).to.deep.equal([child]);
    });

    it('setParentWindow() attaches and detaches a parent at runtime', () => {
      const parent = new BaseWindow({ show: false });
      const w = new BaseWindow({ show: false });
      w.setParentWindow(parent);
      expect(w.getParentWindow()).to.equal(parent);
      expect(parent.getChildWindows()).to.deep.equal([w]);
      w.setParentWindow(null);
      expect(w.getParentWindow()).to.be.null('parent window');
      expect(parent.getChildWindows()).to.have.lengthOf(0);
    });
  });

  describe('visibility', () => {
    let w: BaseWindow;
    beforeEach(() => {
      w = new BaseWindow({ show: false });
    });
    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BaseWindow;
    });

    describe('BaseWindow.show()', () => {
      it('should make the window visible', async () => {
        const show = once(w, 'show');
        w.show();
        await show;
        expect(w.isVisible()).to.be.true('is visible');
      });
    });

    describe('BaseWindow.hide()', () => {
      it('should make the window not visible', async () => {
        const shown = once(w, 'show');
        w.show();
        await shown;
        const hidden = once(w, 'hide');
        w.hide();
        await hidden;
        expect(w.isVisible()).to.be.false('is visible');
      });
    });
  });

  // Wayland does not allow focus and z-order to be controlled without user input
  ifdescribe(!isWayland)('focus and blur', () => {
    let w: BaseWindow;
    beforeEach(() => {
      w = new BaseWindow({ show: false });
    });
    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BaseWindow;
    });

    describe('BaseWindow.show()', () => {
      it('should focus on the window', async () => {
        const focused = once(w, 'focus');
        w.show();
        await focused;
        expect(w.isFocused()).to.be.true('is focused');
      });
    });

    describe('BaseWindow.hide()', () => {
      it('should defocus the window', () => {
        w.hide();
        expect(w.isFocused()).to.be.false('is focused');
      });
    });

    describe('BaseWindow.showInactive()', () => {
      it('should not focus on the window', () => {
        w.showInactive();
        expect(w.isFocused()).to.be.false('is focused');
      });
    });

    ifdescribe(process.platform !== 'win32')('BaseWindow.blur()', () => {
      it('removes focus from the window', async () => {
        const focused = once(w, 'focus');
        w.show();
        await focused;
        const blurred = once(w, 'blur');
        w.blur();
        await blurred;
        expect(w.isFocused()).to.be.false('is focused');
      });
    });

    describe('BaseWindow.getFocusedWindow()', () => {
      it('returns the focused window', async () => {
        const focused = once(w, 'focus');
        w.show();
        await focused;
        expect(BaseWindow.getFocusedWindow()).to.equal(w);
      });
    });
  });

  describe('sizing', () => {
    let w: BaseWindow;

    beforeEach(() => {
      w = new BaseWindow({ show: false, width: 400, height: 400 });
    });

    afterEach(async () => {
      await closeWindow(w);
      w = null as unknown as BaseWindow;
    });

    describe('BaseWindow.setBounds(bounds[, animate])', () => {
      it('sets the window bounds with full bounds', () => {
        const fullBounds = { x: 440, y: 225, width: 500, height: 400 };
        w.setBounds(fullBounds);
        expectBoundsEqual(w.getBounds(), fullBounds);
      });

      it('rounds non-integer bounds', () => {
        w.setBounds({ x: 440.5, y: 225.1, width: 500.4, height: 400.9 });

        const bounds = w.getBounds();
        expect(bounds).to.deep.equal({ x: 441, y: 225, width: 500, height: 401 });
      });

      it('does not emit the resize event for move-only changes', async () => {
        const { x, y, width, height } = w.getBounds();

        w.once('resize', () => {
          expect.fail('resize event should not be emitted');
        });

        w.setBounds({ x: x + 10, y: y + 10, width, height });
      });
    });

    describe('BaseWindow.setSize(width, height)', () => {
      it('sets the window size and emits the resize event', async () => {
        const size = [300, 400];

        const resized = once(w, 'resize');
        w.setSize(size[0], size[1]);
        await resized;

        expectBoundsEqual(w.getSize(), size);
      });
    });

    describe('BaseWindow.setPosition(x, y)', () => {
      it('sets the window position and emits the move event', async () => {
        const pos = [10, 10];
        const move = once(w, 'move');
        w.setPosition(pos[0], pos[1]);
        await move;
        expect(w.getPosition()).to.deep.equal(pos);
      });
    });

    describe('BaseWindow.getNormalBounds()', () => {
      it('matches getBounds() when the window is in normal state', async () => {
        const size = [300, 400];
        const resize = once(w, 'resize');
        w.setSize(size[0], size[1]);
        await resize;
        expectBoundsEqual(w.getNormalBounds(), w.getBounds());
      });
    });

    describe('BaseWindow.setMinimum/MaximumSize(width, height)', () => {
      it('sets the maximum and minimum size of the window', () => {
        expect(w.getMinimumSize()).to.deep.equal([0, 0]);
        expect(w.getMaximumSize()).to.deep.equal([0, 0]);

        w.setMinimumSize(100, 100);
        expectBoundsEqual(w.getMinimumSize(), [100, 100]);
        expectBoundsEqual(w.getMaximumSize(), [0, 0]);

        w.setMaximumSize(900, 600);
        expectBoundsEqual(w.getMinimumSize(), [100, 100]);
        expectBoundsEqual(w.getMaximumSize(), [900, 600]);
      });

      it('creates the window at min size when a smaller size is requested', () => {
        const w1 = new BaseWindow({
          show: false,
          width: 200,
          height: 200,
          minWidth: 300,
          minHeight: 300
        });
        const size = w1.getSize();
        expect(size[0]).to.equal(300);
        expect(size[1]).to.equal(300);
        w1.destroy();
      });

      it('enforces minimum size', async () => {
        w.setMinimumSize(300, 300);
        const resize = once(w, 'resize');
        w.setSize(100, 100);
        await resize;
        const size = w.getSize();
        expect(size[0]).to.be.at.least(300);
        expect(size[1]).to.be.at.least(300);
      });

      it('enforces maximum size', async () => {
        w.setMaximumSize(200, 200);
        const resize = once(w, 'resize');
        w.setSize(500, 500);
        await resize;
        const size = w.getSize();
        expect(size[0]).to.be.at.most(200);
        expect(size[1]).to.be.at.most(200);
      });
    });

    describe('BaseWindow.setContentSize(width, height)', () => {
      it('sets the content size', async () => {
        // NB. The CI server has a very small screen. Attempting to size the window
        // larger than the screen will limit the window's size to the screen and
        // cause the test to fail.
        const size = [456, 567];
        w.setContentSize(size[0], size[1]);
        await new Promise(setImmediate);
        expect(w.getContentSize()).to.deep.equal(size);
      });
    });

    describe('BaseWindow.setContentBounds(bounds)', () => {
      it('sets the content bounds', async () => {
        const bounds = { x: 60, y: 80, width: 400, height: 350 };
        w.setContentBounds(bounds);
        await new Promise(setImmediate);
        expectBoundsEqual(w.getContentBounds(), bounds);
      });
    });

    describe('BaseWindow.setAspectRatio(ratio)', () => {
      it('resets the behaviour when passing in 0', async () => {
        const size = [300, 400];
        w.setAspectRatio(1 / 2);
        w.setAspectRatio(0);
        const resize = once(w, 'resize');
        w.setSize(size[0], size[1]);
        await resize;
        expectBoundsEqual(w.getSize(), size);
      });
    });

    ifdescribe(process.platform !== 'darwin')('BaseWindow.center()', () => {
      it('moves the window to the center and preserves its size', () => {
        const { workArea } = screen.getDisplayMatching(w.getBounds());
        w.setPosition(workArea.x, workArea.y);
        w.center();
        const [width, height] = w.getSize();
        expectBoundsEqual(w.getBounds(), {
          x: workArea.x + Math.floor((workArea.width - width) / 2),
          y: workArea.y + Math.floor((workArea.height - height) / 2),
          width,
          height
        });
      });
    });
  });

  describe('BaseWindow.isNormal()', () => {
    afterEach(closeAllWindows);

    it('returns true when the window is in normal state', async () => {
      const w = new BaseWindow({ show: false });
      expect(w.isNormal()).to.be.true('isNormal');
      const shown = once(w, 'show');
      w.show();
      await shown;
      expect(w.isNormal()).to.be.true('isNormal');
    });
  });

  describe('window states', () => {
    afterEach(closeAllWindows);

    describe('resizable state', () => {
      it('can be set with the resizable constructor option', () => {
        const w = new BaseWindow({ show: false, resizable: false });
        expect(w.resizable).to.be.false('resizable');
      });

      it('can be changed with the property', () => {
        const w = new BaseWindow({ show: false });
        expect(w.resizable).to.be.true('resizable');
        w.resizable = false;
        expect(w.resizable).to.be.false('resizable');
        w.resizable = true;
        expect(w.resizable).to.be.true('resizable');
      });

      it('can be changed with the functions', () => {
        const w = new BaseWindow({ show: false });
        expect(w.isResizable()).to.be.true('resizable');
        w.setResizable(false);
        expect(w.isResizable()).to.be.false('resizable');
        w.setResizable(true);
        expect(w.isResizable()).to.be.true('resizable');
      });
    });

    describe('hasShadow state', () => {
      it('returns a boolean on all platforms', () => {
        const w = new BaseWindow({ show: false });
        expect(w.hasShadow()).to.be.a('boolean');
        expect(w.shadow).to.be.a('boolean');
      });

      // On Windows there's no shadow by default & it can't be changed dynamically.
      it('can be changed with the setHasShadow method', () => {
        const w = new BaseWindow({ show: false });
        w.setHasShadow(false);
        expect(w.hasShadow()).to.be.false('hasShadow');
        w.setHasShadow(true);
        expect(w.hasShadow()).to.be.true('hasShadow');
        w.setHasShadow(false);
        expect(w.hasShadow()).to.be.false('hasShadow');
      });
    });

    describe('fullScreenable state', () => {
      it('can be changed', () => {
        const w = new BaseWindow({ show: false });
        w.setFullScreenable(false);
        expect(w.isFullScreenable()).to.be.false('isFullScreenable');
        w.setFullScreenable(true);
        expect(w.isFullScreenable()).to.be.true('isFullScreenable');
      });
    });

    describe('enabled state', () => {
      it('can be changed with setEnabled', () => {
        const w = new BaseWindow({ show: false });
        expect(w.isEnabled()).to.be.true('isEnabled');
        w.setEnabled(false);
        expect(w.isEnabled()).to.be.false('isEnabled');
        w.setEnabled(true);
        expect(w.isEnabled()).to.be.true('isEnabled');
      });
    });

    // On Linux these setters are no-ops and the getters always return true.
    ifdescribe(process.platform === 'linux')('minimizable/maximizable/closable/movable state on Linux', () => {
      it('the getters return true and the setters are no-ops', () => {
        const w = new BaseWindow({ show: false });
        for (const state of ['minimizable', 'maximizable', 'closable', 'movable'] as const) {
          expect(w[state]).to.be.true(state);
          w[state] = false;
          expect(w[state]).to.be.true(state);
        }
        w.setMinimizable(false);
        expect(w.isMinimizable()).to.be.true('isMinimizable');
        w.setMaximizable(false);
        expect(w.isMaximizable()).to.be.true('isMaximizable');
        w.setClosable(false);
        expect(w.isClosable()).to.be.true('isClosable');
        w.setMovable(false);
        expect(w.isMovable()).to.be.true('isMovable');
      });
    });
  });

  ifdescribe(process.platform !== 'darwin')('menu bar', () => {
    afterEach(closeAllWindows);

    describe('autoHideMenuBar state', () => {
      it('can be set with the autoHideMenuBar constructor option', () => {
        const w = new BaseWindow({ show: false, autoHideMenuBar: true });
        expect(w.autoHideMenuBar).to.be.true('autoHideMenuBar');
        expect(w.isMenuBarAutoHide()).to.be.true('isMenuBarAutoHide');
      });

      it('can be changed', () => {
        const w = new BaseWindow({ show: false });
        expect(w.autoHideMenuBar).to.be.false('autoHideMenuBar');
        w.setAutoHideMenuBar(true);
        expect(w.isMenuBarAutoHide()).to.be.true('isMenuBarAutoHide');
        w.autoHideMenuBar = false;
        expect(w.isMenuBarAutoHide()).to.be.false('isMenuBarAutoHide');
      });
    });

    describe('menuBarVisible state', () => {
      it('can be changed', () => {
        const w = new BaseWindow({ show: false });
        expect(w.menuBarVisible).to.be.true('menuBarVisible');
        w.setMenuBarVisibility(false);
        expect(w.isMenuBarVisible()).to.be.false('isMenuBarVisible');
        w.setMenuBarVisibility(true);
        expect(w.isMenuBarVisible()).to.be.true('isMenuBarVisible');
      });
    });

    describe('BaseWindow.setMenu(menu) and BaseWindow.removeMenu()', () => {
      it('sets and removes the window menu bar', () => {
        const w = new BaseWindow({ show: false });
        const menu = Menu.buildFromTemplate([{ label: 'Test', submenu: [{ label: 'Item' }] }]);
        expect(() => {
          w.setMenu(menu);
          w.removeMenu();
        }).to.not.throw();
      });

      it('does not throw when passing null', () => {
        const w = new BaseWindow({ show: false });
        expect(() => {
          w.setMenu(null);
        }).to.not.throw();
      });
    });
  });

  // Not supported on Wayland.
  ifdescribe(!isWayland)('BaseWindow.setAlwaysOnTop(flag)', () => {
    afterEach(closeAllWindows);

    it('sets the window as always on top', () => {
      const w = new BaseWindow({ show: false });
      expect(w.isAlwaysOnTop()).to.be.false('isAlwaysOnTop');
      w.setAlwaysOnTop(true);
      expect(w.isAlwaysOnTop()).to.be.true('isAlwaysOnTop');
      w.setAlwaysOnTop(false);
      expect(w.isAlwaysOnTop()).to.be.false('isAlwaysOnTop');
    });

    it('causes the right value to be emitted on `always-on-top-changed`', async () => {
      const w = new BaseWindow({ show: false });
      const alwaysOnTopChanged = once(w, 'always-on-top-changed') as Promise<[any, boolean]>;
      w.setAlwaysOnTop(true);
      const [, alwaysOnTop] = await alwaysOnTopChanged;
      expect(alwaysOnTop).to.be.true('alwaysOnTop');
    });
  });

  describe('BaseWindow.flashFrame(flag)', () => {
    afterEach(closeAllWindows);

    it('does not throw when starting and stopping flashing', () => {
      const w = new BaseWindow({ show: false });
      expect(() => {
        w.flashFrame(true);
        w.flashFrame(false);
      }).to.not.throw();
    });
  });

  describe('BaseWindow.getBackgroundColor()', () => {
    afterEach(closeAllWindows);

    it('returns the value set with the backgroundColor option', () => {
      const backgroundColor = '#BBAAFF';
      const w = new BaseWindow({ show: false, backgroundColor });
      expect(w.getBackgroundColor()).to.equal(backgroundColor);
    });

    it('returns the value set with setBackgroundColor()', () => {
      const backgroundColor = '#AABBFF';
      const w = new BaseWindow({ show: false });
      w.setBackgroundColor(backgroundColor);
      expect(w.getBackgroundColor()).to.equal(backgroundColor);
    });
  });

  describe('BaseWindow.setOpacity(opacity)', () => {
    afterEach(closeAllWindows);

    it('makes a window with initial opacity', () => {
      const w = new BaseWindow({ show: false, opacity: 0.5 });
      expect(w.getOpacity()).to.equal(0.5);
    });

    it('allows setting the opacity', () => {
      const w = new BaseWindow({ show: false });
      expect(() => {
        w.setOpacity(0.0);
        expect(w.getOpacity()).to.equal(0.0);
        w.setOpacity(0.5);
        expect(w.getOpacity()).to.equal(0.5);
        w.setOpacity(1.0);
        expect(w.getOpacity()).to.equal(1.0);
      }).to.not.throw();
    });

    it('clamps opacity to [0.0...1.0]', () => {
      const w = new BaseWindow({ show: false, opacity: 0.5 });
      w.setOpacity(100);
      expect(w.getOpacity()).to.equal(1.0);
      w.setOpacity(-100);
      expect(w.getOpacity()).to.equal(0.0);
    });
  });

  ifdescribe(process.platform !== 'darwin')('BaseWindow.setShape(rects)', () => {
    afterEach(closeAllWindows);

    it('allows setting shape', () => {
      const w = new BaseWindow({ show: false });
      expect(() => {
        w.setShape([]);
        w.setShape([{ x: 0, y: 0, width: 100, height: 100 }]);
        w.setShape([
          { x: 0, y: 0, width: 100, height: 100 },
          { x: 0, y: 200, width: 1000, height: 100 }
        ]);
        w.setShape([]);
      }).to.not.throw();
    });
  });

  describe('BaseWindow.setProgressBar(progress)', () => {
    afterEach(closeAllWindows);

    it('sets the progress', () => {
      const w = new BaseWindow({ show: false });
      expect(() => {
        w.setProgressBar(0.5);
        w.setProgressBar(-1);
      }).to.not.throw();
    });

    it('sets the progress using "paused" mode', () => {
      const w = new BaseWindow({ show: false });
      expect(() => {
        w.setProgressBar(0.5, { mode: 'paused' });
      }).to.not.throw();
    });
  });

  describe('BaseWindow.setIgnoreMouseEvents(ignore)', () => {
    afterEach(closeAllWindows);

    it('does not throw', () => {
      const w = new BaseWindow({ show: false });
      expect(() => {
        w.setIgnoreMouseEvents(true);
        w.setIgnoreMouseEvents(false);
      }).to.not.throw();
    });
  });

  describe('BaseWindow.getMediaSourceId()', () => {
    afterEach(closeAllWindows);

    it('returns a valid source id', async () => {
      const w = new BaseWindow({ show: false });
      const shown = once(w, 'show');
      w.show();
      await shown;

      // Check format 'window:1234:0'.
      const sourceId = w.getMediaSourceId();
      expect(sourceId).to.match(/^window:\d+:\d+$/);
    });
  });

  describe('BaseWindow.getNativeWindowHandle()', () => {
    afterEach(closeAllWindows);

    it('returns a non-empty buffer', () => {
      const w = new BaseWindow({ show: false });
      const handle = w.getNativeWindowHandle();
      expect(handle).to.be.an.instanceOf(Buffer);
      expect(handle.length).to.be.greaterThan(0);
    });
  });

  ifdescribe(!isWayland)('BaseWindow.moveTop()', () => {
    afterEach(closeAllWindows);

    it('does not throw or steal focus', async () => {
      const w = new BaseWindow({ show: false });
      const shown = once(w, 'show');
      w.showInactive();
      await shown;
      expect(() => {
        w.moveTop();
      }).to.not.throw();
      expect(w.isFocused()).to.be.false('is focused');
    });
  });

  describe('BaseWindow.moveAbove(mediaSourceId)', () => {
    afterEach(closeAllWindows);

    it('should throw an exception if wrong formatting', () => {
      const w = new BaseWindow({ show: false });
      const fakeSourceIds = ['none', 'screen:0', 'window:fake', 'window:1234', 'foobar:1:2'];
      for (const sourceId of fakeSourceIds) {
        expect(() => {
          w.moveAbove(sourceId);
        }).to.throw(/Invalid media source id/);
      }
    });
  });

  describe('window.title', () => {
    afterEach(closeAllWindows);

    it('can be set with the title constructor option', () => {
      const w = new BaseWindow({ show: false, title: 'mYtItLe' });
      expect(w.title).to.equal('mYtItLe');
      expect(w.getTitle()).to.equal('mYtItLe');
    });

    it('can be changed', () => {
      const w = new BaseWindow({ show: false });
      w.setTitle('NEW TITLE');
      expect(w.getTitle()).to.equal('NEW TITLE');
      w.title = 'ANOTHER TITLE';
      expect(w.title).to.equal('ANOTHER TITLE');
    });
  });

  describe('window.accessibleTitle', () => {
    afterEach(closeAllWindows);

    it('can be set and retrieved', () => {
      const w = new BaseWindow({ show: false, title: 'mYtItLe' });
      expect(w.accessibleTitle).to.equal('mYtItLe');
      w.accessibleTitle = 'accessible title';
      expect(w.accessibleTitle).to.equal('accessible title');
      w.accessibleTitle = '';
      expect(w.accessibleTitle).to.equal('mYtItLe');
    });
  });
});
