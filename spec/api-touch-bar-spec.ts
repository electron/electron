import { BaseWindow, BrowserWindow, TouchBar } from 'electron/main';

import { expect } from 'chai';

import * as path from 'node:path';

import { closeWindow } from './lib/window-helpers';

const { TouchBarButton, TouchBarColorPicker, TouchBarGroup, TouchBarLabel, TouchBarOtherItemsProxy, TouchBarPopover, TouchBarScrubber, TouchBarSegmentedControl, TouchBarSlider, TouchBarSpacer } = TouchBar;

describe('TouchBar module', () => {
  it('throws an error when created without an options object', () => {
    expect(() => {
      const touchBar = new (TouchBar as any)();
      touchBar.toString();
    }).to.throw('Must specify options object as first argument');
  });

  it('throws an error when created with invalid items', () => {
    expect(() => {
      const touchBar = new TouchBar({ items: [1, true, {}, []] as any });
      touchBar.toString();
    }).to.throw('Each item must be an instance of TouchBarItem');
  });

  it('throws an error when an invalid escape item is set', () => {
    expect(() => {
      const touchBar = new TouchBar({ items: [], escapeItem: 'esc' as any });
      touchBar.toString();
    }).to.throw('Escape item must be an instance of TouchBarItem');

    expect(() => {
      const touchBar = new TouchBar({ items: [] });
      touchBar.escapeItem = 'esc' as any;
    }).to.throw('Escape item must be an instance of TouchBarItem');
  });

  it('throws an error if multiple OtherItemProxy items are added', () => {
    expect(() => {
      const touchBar = new TouchBar({ items: [new TouchBarOtherItemsProxy(), new TouchBarOtherItemsProxy()] });
      touchBar.toString();
    }).to.throw('Must only have one OtherItemsProxy per TouchBar');
  });

  it('throws an error if the same TouchBarItem is added multiple times', () => {
    expect(() => {
      const item = new TouchBarLabel({ label: 'Label' });
      const touchBar = new TouchBar({ items: [item, item] });
      touchBar.toString();
    }).to.throw('Cannot add a single instance of TouchBarItem multiple times in a TouchBar');
  });

  describe('Window behavior', () => {
    for (const WindowType of [BrowserWindow, BaseWindow]) {
      describe(`in ${WindowType.name}`, () => {
        let window: BaseWindow | BrowserWindow;

        beforeEach(() => {
          window = new WindowType({ show: false });
        });

        afterEach(async () => {
          window.setTouchBar(null);
          await closeWindow(window);
          window = null as unknown as BaseWindow | BrowserWindow;
        });

        it('can be added to and removed from a window', () => {
          const label = new TouchBarLabel({ label: 'bar' });
          const touchBar = new TouchBar({
            items: [
              new TouchBarButton({ label: 'foo', backgroundColor: '#F00', click: () => { } }),
              new TouchBarButton({
                icon: path.join(__dirname, 'fixtures', 'assets', 'logo.png'),
                iconPosition: 'right',
                click: () => { }
              }),
              new TouchBarColorPicker({ selectedColor: '#F00', change: () => { } }),
              new TouchBarGroup({ items: new TouchBar({ items: [new TouchBarLabel({ label: 'hello' })] }) }),
              label,
              new TouchBarOtherItemsProxy(),
              new TouchBarPopover({ items: new TouchBar({ items: [new TouchBarButton({ label: 'pop' })] }) }),
              new TouchBarSlider({ label: 'slide', value: 5, minValue: 2, maxValue: 75, change: () => { } }),
              new TouchBarSpacer({ size: 'large' }),
              new TouchBarSegmentedControl({
                segmentStyle: 'capsule',
                segments: [{ label: 'baz', enabled: false }],
                selectedIndex: 5
              }),
              new TouchBarSegmentedControl({ segments: [] }),
              new TouchBarScrubber({
                items: [{ label: 'foo' }, { label: 'bar' }, { label: 'baz' }],
                selectedStyle: 'outline',
                mode: 'fixed',
                showArrowButtons: true
              })
            ]
          });
          const escapeButton = new TouchBarButton({ label: 'foo' });
          window.setTouchBar(touchBar);
          touchBar.escapeItem = escapeButton;
          label.label = 'baz';
          escapeButton.label = 'hello';
          window.setTouchBar(null);
          window.setTouchBar(new TouchBar({ items: [new TouchBarLabel({ label: 'two' })] }));
          touchBar.escapeItem = null;
        });

        it('calls the callback on the items when a window interaction event fires', (done) => {
          const button = new TouchBarButton({
            label: 'bar',
            click: () => {
              done();
            }
          });
          const touchBar = new TouchBar({ items: [button] });
          window.setTouchBar(touchBar);
          window.emit('-touch-bar-interaction', {}, (button as any).id);
        });

        it('calls the callback on the escape item when a window interaction event fires', (done) => {
          const button = new TouchBarButton({
            label: 'bar',
            click: () => {
              done();
            }
          });
          const touchBar = new TouchBar({ escapeItem: button });
          window.setTouchBar(touchBar);
          window.emit('-touch-bar-interaction', {}, (button as any).id);
        });
      });
    };
  });
});
