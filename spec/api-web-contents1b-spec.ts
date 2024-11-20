import { BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import { ifdescribe } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const features = process._linkedBinding('electron_common_features');

describe('webContents1b module', () => {
  ifdescribe(features.isPrintingEnabled())('webContents.print()', () => {
    let w: BrowserWindow;

    beforeEach(() => {
      w = new BrowserWindow({ show: false });
    });

    afterEach(closeAllWindows);

    it('does not throw when options are not passed', () => {
      expect(() => {
        w.webContents.print();
      }).not.to.throw();

      expect(() => {
        w.webContents.print(undefined);
      }).not.to.throw();
    });

    it('does not throw when options object is empty', () => {
      expect(() => {
        w.webContents.print({});
      }).not.to.throw();
    });

    it('throws when invalid settings are passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print(true);
      }).to.throw('webContents.print(): Invalid print settings specified.');

      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print(null);
      }).to.throw('webContents.print(): Invalid print settings specified.');
    });

    it('throws when an invalid pageSize is passed', () => {
      const badSize = 5;
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({ pageSize: badSize });
      }).to.throw(`Unsupported pageSize: ${badSize}`);
    });

    it('throws when an invalid callback is passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({}, true);
      }).to.throw('webContents.print(): Invalid optional callback provided.');
    });

    it('fails when an invalid deviceName is passed', (done) => {
      w.webContents.print({ deviceName: 'i-am-a-nonexistent-printer' }, (success, reason) => {
        expect(success).to.equal(false);
        expect(reason).to.match(/Invalid deviceName provided/);
        done();
      });
    });

    it('throws when an invalid pageSize is passed', () => {
      expect(() => {
        // @ts-ignore this line is intentionally incorrect
        w.webContents.print({ pageSize: 'i-am-a-bad-pagesize' }, () => {});
      }).to.throw('Unsupported pageSize: i-am-a-bad-pagesize');
    });

    it('throws when an invalid custom pageSize is passed', () => {
      expect(() => {
        w.webContents.print({
          pageSize: {
            width: 100,
            height: 200
          }
        });
      }).to.throw('height and width properties must be minimum 352 microns.');
    });

    it('does not crash with custom margins', () => {
      expect(() => {
        w.webContents.print({
          silent: true,
          margins: {
            marginType: 'custom',
            top: 1,
            bottom: 1,
            left: 1,
            right: 1
          }
        });
      }).to.not.throw();
    });
  });
});
