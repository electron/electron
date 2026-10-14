import { nativeImage } from 'electron/common';
import { BrowserWindow } from 'electron/main';

import { afterEach, describe, expect, it } from 'vitest';

import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { ifdescribe, ifit, itremote, useRemoteContext } from './lib/spec-helpers.ts';
import { expectDeprecationMessages } from './lib/warning-helpers.ts';
import { closeAllWindows } from './lib/window-helpers.ts';

describe('nativeImage module', () => {
  const fixturesPath = path.join(import.meta.dirname, 'fixtures');

  const imageLogo = {
    path: path.join(fixturesPath, 'assets', 'logo.png'),
    width: 538,
    height: 190
  };
  const image1x1 = {
    dataUrl:
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADElEQVR4nGJiAAIAAAAA//81yQwvAAAABklEQVQDAAAPAAMkN33pAAAAAElFTkSuQmCC',
    path: path.join(fixturesPath, 'assets', '1x1.png'),
    height: 1,
    width: 1
  };
  const image2x2 = {
    dataUrl:
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAAE0lEQVR4nGL5//8/AwMDEwMYAAAAAP//aYxtrAAAAAZJREFUAwAkMAMEkRkhTQAAAABJRU5ErkJggg==',
    path: path.join(fixturesPath, 'assets', '2x2.jpg'),
    height: 2,
    width: 2
  };
  const image3x3 = {
    dataUrl:
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAMAAAADCAYAAABWKLW/AAABCWlDQ1BfAAB4nJWQsUrDUBSGv6oggqCDg4PDHVyVakEcnKoQXGOE6pbEGIUkDUlKVxdxFsRZxGfQBxFdXPoIDuLsfxswXTr0XM49H+f+nPtzoDVCsdCGNKsK1+ma3tm5WRzR0hmHH5Y500Oq389a+77F7LF0EZWh6rfSK/S5Rh6J1+Kae5aDmhPLwyqvxDeWC889FD+IV+MJDiY4zAurfxEfpMkgbHyzHGWnJ6r7yg1KXBy6GN0+KQMq8ZBr1SvRLttKg0eh90z6SyKsZVOP7D9p1A/M3ze94BHe7mD9q+ltPsPKLbx+NL1mh7lf+P/bmet0png2Y88OfZ2YRE4Mx3IVyqX12maHvT9v1kU+i+h9qQAAAA9JREFUeJxiYkACTDg5AAAAAP//rSd/1QAAAAZJREFUAwAAwwAHu+/W1AAAAABJRU5ErkJggg==',
    path: path.join(fixturesPath, 'assets', '3x3.png'),
    height: 3,
    width: 3
  };
  const imageColorSpaceSRGB = {
    path: path.join(fixturesPath, 'assets', 'colorspace-srgb.png')
  };
  const imageColorSpaceP3 = {
    path: path.join(fixturesPath, 'assets', 'colorspace-p3.png')
  };

  const dataUrlImages = [image1x1, image2x2, image3x3];

  ifdescribe(process.platform === 'darwin')('isMacTemplateImage state', () => {
    describe('with properties', () => {
      it('correctly recognizes a template image', () => {
        const image = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));
        expect(image.isMacTemplateImage).toBe(false);

        const templateImage = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo_Template.png'));
        expect(templateImage.isMacTemplateImage).toBe(true);
      });

      it('sets a template image', () => {
        const image = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));
        expect(image.isMacTemplateImage).toBe(false);

        image.isMacTemplateImage = true;
        expect(image.isMacTemplateImage).toBe(true);
      });
    });

    describe('with functions', () => {
      it('correctly recognizes a template image', () => {
        const image = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));
        expect(image.isTemplateImage()).toBe(false);

        const templateImage = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo_Template.png'));
        expect(templateImage.isTemplateImage()).toBe(true);
      });

      it('sets a template image', () => {
        const image = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));
        expect(image.isTemplateImage()).toBe(false);

        image.setTemplateImage(true);
        expect(image.isTemplateImage()).toBe(true);
      });
    });
  });

  describe('createEmpty()', () => {
    it('returns an empty image', async () => {
      const empty = nativeImage.createEmpty();
      expect(empty.isEmpty()).toBe(true);
      expect(empty.getAspectRatio()).toBe(1);
      expect(empty.toDataURL()).toBe('data:image/png;base64,');
      expect(empty.toDataURL({ scaleFactor: 2.0 })).toBe('data:image/png;base64,');
      expect(empty.getSize()).toEqual({ width: 0, height: 0 });
      await expectDeprecationMessages(() => {
        expect(empty.getBitmap()).toHaveLength(0);
        expect(empty.getBitmap({ scaleFactor: 2.0 })).toHaveLength(0);
      }, 'getBitmap() is deprecated, use toBitmap() instead.');
      expect(empty.toBitmap()).toHaveLength(0);
      expect(empty.toBitmap({ scaleFactor: 2.0 })).toHaveLength(0);
      expect(empty.toJPEG(100)).toHaveLength(0);
      expect(empty.toPNG()).toHaveLength(0);
      expect(empty.toPNG({ scaleFactor: 2.0 })).toHaveLength(0);

      if (process.platform === 'darwin') {
        expect(empty.getNativeHandle()).toHaveLength(0);
      }
    });
  });

  describe('createFromBitmap(buffer, options)', () => {
    it('returns an empty image when the buffer is empty', () => {
      expect(nativeImage.createFromBitmap(Buffer.from([]), { width: 0, height: 0 }).isEmpty()).toBe(true);
    });

    it('returns an image created from the given buffer', () => {
      const imageA = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));

      const imageB = nativeImage.createFromBitmap(imageA.toBitmap(), imageA.getSize());
      expect(imageB.getSize()).toEqual({ width: 538, height: 190 });

      const imageC = nativeImage.createFromBuffer(imageA.toBitmap(), { ...imageA.getSize(), scaleFactor: 2.0 });
      expect(imageC.getSize()).toEqual({ width: 269, height: 95 });
    });

    it('throws on invalid arguments', () => {
      expect(() => nativeImage.createFromBitmap(null as any, {} as any)).toThrow('buffer must be a node Buffer');
      expect(() => nativeImage.createFromBitmap([12, 14, 124, 12] as any, {} as any)).toThrow(
        'buffer must be a node Buffer'
      );
      expect(() => nativeImage.createFromBitmap(Buffer.from([]), {} as any)).toThrow('width is required');
      expect(() => nativeImage.createFromBitmap(Buffer.from([]), { width: 1 } as any)).toThrow('height is required');
      expect(() => nativeImage.createFromBitmap(Buffer.from([]), { width: 1, height: 1 })).toThrow(
        'invalid buffer size'
      );
    });
  });

  describe('createFromBuffer(buffer, options)', () => {
    it('returns an empty image when the buffer is empty', () => {
      expect(nativeImage.createFromBuffer(Buffer.from([])).isEmpty()).toBe(true);
    });

    it('returns an empty image when the buffer is too small', () => {
      const image = nativeImage.createFromBuffer(Buffer.from([1, 2, 3, 4]), { width: 100, height: 100 });
      expect(image.isEmpty()).toBe(true);
    });

    it('returns an image created from the given buffer', () => {
      const imageA = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));

      const imageB = nativeImage.createFromBuffer(imageA.toPNG());
      expect(imageB.getSize()).toEqual({ width: 538, height: 190 });
      expect(imageA.toBitmap().equals(imageB.toBitmap())).toBe(true);

      const imageC = nativeImage.createFromBuffer(imageA.toJPEG(100));
      expect(imageC.getSize()).toEqual({ width: 538, height: 190 });

      const imageD = nativeImage.createFromBuffer(imageA.toBitmap(), { width: 538, height: 190 });
      expect(imageD.getSize()).toEqual({ width: 538, height: 190 });

      const imageE = nativeImage.createFromBuffer(imageA.toBitmap(), { width: 100, height: 200 });
      expect(imageE.getSize()).toEqual({ width: 100, height: 200 });

      const imageF = nativeImage.createFromBuffer(imageA.toBitmap());
      expect(imageF.isEmpty()).toBe(true);

      const imageG = nativeImage.createFromBuffer(imageA.toPNG(), { width: 100, height: 200 });
      expect(imageG.getSize()).toEqual({ width: 538, height: 190 });

      const imageH = nativeImage.createFromBuffer(imageA.toJPEG(100), { width: 100, height: 200 });
      expect(imageH.getSize()).toEqual({ width: 538, height: 190 });

      const imageI = nativeImage.createFromBuffer(imageA.toBitmap(), { width: 538, height: 190, scaleFactor: 2.0 });
      expect(imageI.getSize()).toEqual({ width: 269, height: 95 });
    });

    it('throws on invalid arguments', () => {
      expect(() => nativeImage.createFromBuffer(null as any)).toThrow('buffer must be a node Buffer');
      expect(() => nativeImage.createFromBuffer([12, 14, 124, 12] as any)).toThrow('buffer must be a node Buffer');
    });
  });

  describe('createFromDataURL(dataURL)', () => {
    it('returns an empty image from the empty string', () => {
      expect(nativeImage.createFromDataURL('').isEmpty()).toBe(true);
    });

    it('returns an image created from the given string', () => {
      for (const imageData of dataUrlImages) {
        const imageFromPath = nativeImage.createFromBuffer(nativeImage.createFromPath(imageData.path).toPNG());
        const imageFromDataUrl = nativeImage.createFromDataURL(imageData.dataUrl!);

        expect(imageFromDataUrl.isEmpty()).toBe(false);
        expect(imageFromDataUrl.getSize()).toEqual(imageFromPath.getSize());
        expect(imageFromDataUrl.toBitmap()).toSatisfy((bitmap: any) => imageFromPath.toBitmap().equals(bitmap));
        expect(imageFromDataUrl.toDataURL()).toBe(imageFromPath.toDataURL());
      }
    });
  });

  describe('toDataURL()', () => {
    it('returns a PNG data URL', () => {
      for (const imageData of dataUrlImages) {
        const imageFromPath = nativeImage.createFromBuffer(nativeImage.createFromPath(imageData.path!).toPNG());

        const scaleFactors = [1.0, 2.0];
        for (const scaleFactor of scaleFactors) {
          expect(imageFromPath.toDataURL({ scaleFactor })).toBe(imageData.dataUrl);
        }
      }
    });

    it('returns a data URL at 1x scale factor by default', () => {
      const imageData = imageLogo;
      const image = nativeImage.createFromPath(imageData.path);

      const imageOne = nativeImage.createFromBuffer(image.toPNG(), {
        width: image.getSize().width,
        height: image.getSize().height,
        scaleFactor: 2.0
      });
      expect(imageOne.getSize()).toEqual({ width: imageData.width / 2, height: imageData.height / 2 });

      const imageTwo = nativeImage.createFromDataURL(imageOne.toDataURL());
      expect(imageTwo.getSize()).toEqual({ width: imageData.width, height: imageData.height });

      expect(imageOne.toBitmap().equals(imageTwo.toBitmap())).toBe(true);
    });

    it('supports a scale factor', () => {
      const imageData = imageLogo;
      const image = nativeImage.createFromPath(imageData.path);
      const expectedSize = { width: imageData.width, height: imageData.height };

      const imageFromDataUrlOne = nativeImage.createFromDataURL(image.toDataURL({ scaleFactor: 1.0 }));
      expect(imageFromDataUrlOne.getSize()).toEqual(expectedSize);

      const imageFromDataUrlTwo = nativeImage.createFromDataURL(image.toDataURL({ scaleFactor: 2.0 }));
      expect(imageFromDataUrlTwo.getSize()).toEqual(expectedSize);
    });
  });

  describe('toPNG()', () => {
    it('returns a buffer at 1x scale factor by default', () => {
      const imageData = imageLogo;
      const imageA = nativeImage.createFromPath(imageData.path);

      const imageB = nativeImage.createFromBuffer(imageA.toPNG(), {
        width: imageA.getSize().width,
        height: imageA.getSize().height,
        scaleFactor: 2.0
      });
      expect(imageB.getSize()).toEqual({ width: imageData.width / 2, height: imageData.height / 2 });

      const imageC = nativeImage.createFromBuffer(imageB.toPNG());
      expect(imageC.getSize()).toEqual({ width: imageData.width, height: imageData.height });

      expect(imageB.toBitmap().equals(imageC.toBitmap())).toBe(true);
    });

    it('supports a scale factor', () => {
      const imageData = imageLogo;
      const image = nativeImage.createFromPath(imageData.path);

      const imageFromBufferOne = nativeImage.createFromBuffer(image.toPNG({ scaleFactor: 1.0 }));
      expect(imageFromBufferOne.getSize()).toEqual({ width: imageData.width, height: imageData.height });

      const imageFromBufferTwo = nativeImage.createFromBuffer(image.toPNG({ scaleFactor: 2.0 }), { scaleFactor: 2.0 });
      expect(imageFromBufferTwo.getSize()).toEqual({ width: imageData.width / 2, height: imageData.height / 2 });
    });
  });

  describe('createFromPath(path)', () => {
    it('returns an empty image for invalid paths', () => {
      expect(nativeImage.createFromPath('').isEmpty()).toBe(true);
      expect(nativeImage.createFromPath('does-not-exist.png').isEmpty()).toBe(true);
      expect(nativeImage.createFromPath('does-not-exist.ico').isEmpty()).toBe(true);
      expect(nativeImage.createFromPath(import.meta.dirname).isEmpty()).toBe(true);
      expect(nativeImage.createFromPath(import.meta.filename).isEmpty()).toBe(true);
    });

    it('loads images from paths relative to the current working directory', () => {
      const imagePath = path.relative('.', path.join(fixturesPath, 'assets', 'logo.png'));
      const image = nativeImage.createFromPath(imagePath);
      expect(image.isEmpty()).toBe(false);
      expect(image.getSize()).toEqual({ width: 538, height: 190 });
    });

    it('loads images from paths with `.` segments', () => {
      const imagePath = `${path.join(fixturesPath)}${path.sep}.${path.sep}${path.join('assets', 'logo.png')}`;
      const image = nativeImage.createFromPath(imagePath);
      expect(image.isEmpty()).toBe(false);
      expect(image.getSize()).toEqual({ width: 538, height: 190 });
    });

    it('loads images from paths with `..` segments', () => {
      const imagePath = `${path.join(fixturesPath, 'api')}${path.sep}..${path.sep}${path.join('assets', 'logo.png')}`;
      const image = nativeImage.createFromPath(imagePath);
      expect(image.isEmpty()).toBe(false);
      expect(image.getSize()).toEqual({ width: 538, height: 190 });
    });

    ifit(process.platform === 'darwin')('Gets an NSImage pointer on macOS', () => {
      const imagePath = `${path.join(fixturesPath, 'api')}${path.sep}..${path.sep}${path.join('assets', 'logo.png')}`;
      const image = nativeImage.createFromPath(imagePath);
      const nsimage = image.getNativeHandle();

      expect(nsimage).toHaveLength(8);

      // If all bytes are null, that's Bad
      const allBytesAreNotNull = nsimage.reduce((acc, x) => acc || x !== 0, false);
      expect(allBytesAreNotNull);
    });

    ifit(process.platform === 'win32')('loads images from .ico files on Windows', () => {
      const imagePath = path.join(fixturesPath, 'assets', 'icon.ico');
      const image = nativeImage.createFromPath(imagePath);
      expect(image.isEmpty()).toBe(false);
      expect(image.getSize()).toEqual({ width: 256, height: 256 });
    });

    ifdescribe(process.platform === 'win32')('when loading .ico files from ASAR archives', { retry: 0 }, () => {
      const archivePath = path.join(fixturesPath, 'test.asar', 'icon.asar');

      // Packed cases use distinct members so a reverted implementation cannot reuse a stale extracted path.
      const withIsolatedTempDirectory = (callback: () => void) => {
        const testDirectory = fs.mkdtempSync(path.join(os.tmpdir(), 'electron-native-image-'));
        const previousTemp = process.env.TEMP;
        const previousTmp = process.env.TMP;
        try {
          process.env.TEMP = testDirectory;
          process.env.TMP = testDirectory;
          const entriesBefore = fs.readdirSync(testDirectory).sort();

          callback();

          expect(fs.readdirSync(testDirectory).sort()).toEqual(entriesBefore);
        } finally {
          if (previousTemp === undefined) delete process.env.TEMP;
          else process.env.TEMP = previousTemp;
          if (previousTmp === undefined) delete process.env.TMP;
          else process.env.TMP = previousTmp;
          fs.rmSync(testDirectory, { recursive: true, force: true });
        }
      };

      const expectValidIcon = (relativePath: string) => {
        const image = nativeImage.createFromPath(path.join(archivePath, relativePath));
        expect(image.isEmpty()).toBe(false);
        expect(image.getSize()).toEqual({ width: 256, height: 256 });
      };

      it('removes temporary files after loading a packed ICO', () => {
        withIsolatedTempDirectory(() => expectValidIcon('icon.ico'));
      });

      it('does not create temporary files for an unpacked ICO', () => {
        withIsolatedTempDirectory(() => expectValidIcon('unpacked.ico'));
      });

      it('does not create temporary files for a missing ICO', () => {
        withIsolatedTempDirectory(() => {
          expect(nativeImage.createFromPath(path.join(archivePath, 'missing.ico')).isEmpty()).toBe(true);
        });
      });

      it('does not create temporary files for a missing ASAR archive', () => {
        withIsolatedTempDirectory(() => {
          const missingArchive = path.join(fixturesPath, 'test.asar', 'missing.asar');
          expect(nativeImage.createFromPath(path.join(missingArchive, 'icon.ico')).isEmpty()).toBe(true);
        });
      });

      it('removes temporary files when a packed ICO is invalid', () => {
        withIsolatedTempDirectory(() => {
          expect(nativeImage.createFromPath(path.join(archivePath, 'invalid.ico')).isEmpty()).toBe(true);
        });
      });

      it('removes temporary files after repeatedly loading the same packed ICO', () => {
        withIsolatedTempDirectory(() => {
          for (let i = 0; i < 3; i++) expectValidIcon('repeated.ico');
        });
      });

      it('removes temporary files after loading multiple packed ICOs', () => {
        withIsolatedTempDirectory(() => {
          expectValidIcon('multiple-one.ico');
          expectValidIcon('multiple-two.ico');
        });
      });

      it('removes temporary files after loading a linked packed ICO', () => {
        withIsolatedTempDirectory(() => expectValidIcon('linked.ico'));
      });
    });
  });

  describe('createFromNamedImage(name)', () => {
    it('returns empty for invalid options', () => {
      const image = nativeImage.createFromNamedImage('totally_not_real');
      expect(image.isEmpty()).toBe(true);
    });

    ifit(process.platform !== 'darwin')('returns empty on non-darwin platforms', () => {
      const image = nativeImage.createFromNamedImage('NSActionTemplate');
      expect(image.isEmpty()).toBe(true);
    });

    ifit(process.platform === 'darwin')('returns a valid image on darwin', () => {
      const image = nativeImage.createFromNamedImage('NSActionTemplate');
      expect(image.isEmpty()).toBe(false);
    });

    ifit(process.platform === 'darwin')('returns a valid named symbol on darwin', () => {
      const image = nativeImage.createFromNamedImage('atom');
      expect(image.isEmpty()).toBe(false);
    });

    ifit(process.platform === 'darwin')('returns a valid named symbol with options on darwin', () => {
      const image = nativeImage.createFromNamedImage('atom', {
        weight: 'ultralight',
        scale: 'small',
        pointSize: 24
      });
      expect(image.isEmpty()).toBe(false);
    });

    ifit(process.platform === 'darwin')('returns allows an HSL shift for a valid image on darwin', () => {
      const image = nativeImage.createFromNamedImage('NSActionTemplate', [0.5, 0.2, 0.8]);
      expect(image.isEmpty()).toBe(false);
    });
  });

  describe('createMenuSymbol(name)', () => {
    it('returns empty for invalid options', () => {
      const image = nativeImage.createMenuSymbol('totally_not_real');
      expect(image.isEmpty()).toBe(true);
    });

    ifit(process.platform === 'darwin')('returns a valid image on darwin', () => {
      const image = nativeImage.createMenuSymbol('atom');
      expect(image.isEmpty()).toBe(false);
    });
  });

  describe('resize(options)', () => {
    it('returns a resized image', () => {
      const image = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));
      for (const [resizeTo, expectedSize] of new Map([
        [{}, { width: 538, height: 190 }],
        [{ width: 269 }, { width: 269, height: 95 }],
        [{ width: 600 }, { width: 600, height: 212 }],
        [{ height: 95 }, { width: 269, height: 95 }],
        [{ height: 200 }, { width: 566, height: 200 }],
        [
          { width: 80, height: 65 },
          { width: 80, height: 65 }
        ],
        [
          { width: 600, height: 200 },
          { width: 600, height: 200 }
        ],
        [
          { width: 0, height: 0 },
          { width: 0, height: 0 }
        ],
        [
          { width: -1, height: -1 },
          { width: 0, height: 0 }
        ]
      ])) {
        const actualSize = image.resize(resizeTo).getSize();
        expect(actualSize).toEqual(expectedSize);
      }
    });

    it('returns an empty image when called on an empty image', () => {
      expect(nativeImage.createEmpty().resize({ width: 1, height: 1 }).isEmpty()).toBe(true);
      expect(nativeImage.createEmpty().resize({ width: 0, height: 0 }).isEmpty()).toBe(true);
    });

    it('supports a quality option', () => {
      const image = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));
      const good = image.resize({ width: 100, height: 100, quality: 'good' });
      const better = image.resize({ width: 100, height: 100, quality: 'better' });
      const best = image.resize({ width: 100, height: 100, quality: 'best' });

      expect(good.toPNG().length).toBeLessThanOrEqual(better.toPNG().length);
      expect(better.toPNG().length).toBeLessThan(best.toPNG().length);
    });
  });

  describe('crop(bounds)', () => {
    it('returns an empty image when called on an empty image', () => {
      expect(nativeImage.createEmpty().crop({ width: 1, height: 2, x: 0, y: 0 }).isEmpty()).toBe(true);
      expect(nativeImage.createEmpty().crop({ width: 0, height: 0, x: 0, y: 0 }).isEmpty()).toBe(true);
    });

    it('returns an empty image when the bounds are invalid', () => {
      const image = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));
      expect(image.crop({ width: 0, height: 0, x: 0, y: 0 }).isEmpty()).toBe(true);
      expect(image.crop({ width: -1, height: 10, x: 0, y: 0 }).isEmpty()).toBe(true);
      expect(image.crop({ width: 10, height: -35, x: 0, y: 0 }).isEmpty()).toBe(true);
      expect(image.crop({ width: 100, height: 100, x: 1000, y: 1000 }).isEmpty()).toBe(true);
    });

    it('returns a cropped image', () => {
      const image = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));
      const cropA = image.crop({ width: 25, height: 64, x: 0, y: 0 });
      const cropB = image.crop({ width: 25, height: 64, x: 30, y: 40 });
      expect(cropA.getSize()).toEqual({ width: 25, height: 64 });
      expect(cropB.getSize()).toEqual({ width: 25, height: 64 });
      expect(cropA.toPNG().equals(cropB.toPNG())).toBe(false);
    });

    it('toBitmap() returns a buffer of the right size', () => {
      const image = nativeImage.createFromPath(path.join(fixturesPath, 'assets', 'logo.png'));
      const crop = image.crop({ width: 25, height: 64, x: 0, y: 0 });
      expect(crop.toBitmap().length).toBe(25 * 64 * 4);
    });

    it('toBitmap() normalizes color space for consistent pixel values', () => {
      const srgbImage = nativeImage.createFromPath(imageColorSpaceSRGB.path);
      const p3Image = nativeImage.createFromPath(imageColorSpaceP3.path);

      // Both default to sRGB normalization
      const srgbBuf = srgbImage.toBitmap();
      const p3Buf = p3Image.toBitmap();

      expect(srgbBuf.length).toBe(p3Buf.length);

      // Pixel values should be nearly identical after sRGB normalization
      for (let i = 0; i < srgbBuf.length; i++) {
        expect(Math.abs(srgbBuf[i] - p3Buf[i])).toBeLessThanOrEqual(3);
      }
    });

    it('toBitmap() accepts a colorSpace option', () => {
      const image = nativeImage.createFromPath(imageColorSpaceP3.path);

      const srgbBuf = image.toBitmap({
        colorSpace: { primaries: 'bt709', transfer: 'srgb', matrix: 'rgb', range: 'full' }
      });
      const p3Buf = image.toBitmap({ colorSpace: { primaries: 'p3', transfer: 'srgb', matrix: 'rgb', range: 'full' } });

      // Both should produce valid buffers of the same size (same pixel dimensions)
      expect(srgbBuf.length).toBeGreaterThan(0);
      expect(p3Buf.length).toBe(srgbBuf.length);
    });
  });

  describe('getAspectRatio()', () => {
    it('returns an aspect ratio of an empty image', () => {
      expect(nativeImage.createEmpty().getAspectRatio()).toBe(1.0);
    });

    it('returns an aspect ratio of an image', () => {
      const imageData = imageLogo;
      // imageData.width / imageData.height = 2.831578947368421
      const expectedAspectRatio = 2.8315789699554443;

      const image = nativeImage.createFromPath(imageData.path);
      expect(image.getAspectRatio()).toBe(expectedAspectRatio);
    });
  });

  ifdescribe(process.platform !== 'linux')('createThumbnailFromPath(path, size)', () => {
    useRemoteContext({ webPreferences: { contextIsolation: false, nodeIntegration: true } });

    it('throws when invalid size is passed', async () => {
      const badSize = { width: -1, height: -1 };

      await expect(nativeImage.createThumbnailFromPath('path', badSize)).rejects.toThrow('size must not be empty');
    });

    it('throws when a bad path is passed', async () => {
      const badPath = process.platform === 'win32' ? '\\hey\\hi\\hello' : '/hey/hi/hello';
      const goodSize = { width: 100, height: 100 };

      await expect(nativeImage.createThumbnailFromPath(badPath, goodSize)).rejects.toThrow();
    });

    it('returns native image given valid params', async () => {
      const goodPath = path.join(fixturesPath, 'assets', 'logo.png');
      const goodSize = { width: 100, height: 100 };
      const result = await nativeImage.createThumbnailFromPath(goodPath, goodSize);
      expect(result.isEmpty()).toBe(false);
    });

    it('returns the correct size if larger than the initial image', async () => {
      // capybara.png is a 128x128 image.
      const imgPath = path.join(fixturesPath, 'assets', 'capybara.png');
      const size = { width: 256, height: 256 };
      const result = await nativeImage.createThumbnailFromPath(imgPath, size);
      expect(result.getSize()).toEqual(size);
    });

    it('returns the correct size if is the same as the initial image', async () => {
      // capybara.png is a 128x128 image.
      const imgPath = path.join(fixturesPath, 'assets', 'capybara.png');
      const size = { width: 128, height: 128 };
      const result = await nativeImage.createThumbnailFromPath(imgPath, size);
      expect(result.getSize()).toEqual(size);
    });

    it('returns the correct size if smaller than the initial image', async () => {
      // capybara.png is a 128x128 image.
      const imgPath = path.join(fixturesPath, 'assets', 'capybara.png');
      const maxSize = { width: 64, height: 64 };
      const result = await nativeImage.createThumbnailFromPath(imgPath, maxSize);
      expect(result.getSize()).toEqual(maxSize);
    });

    itremote(
      'works in the renderer',
      async (path: string) => {
        const { nativeImage } = require('electron');
        const goodSize = { width: 100, height: 100 };
        const result = await nativeImage.createThumbnailFromPath(path, goodSize);
        expect(result.isEmpty()).toBe(false);
      },
      [path.join(fixturesPath, 'assets', 'logo.png')]
    );
  });

  describe('addRepresentation()', () => {
    it('does not add representation when the buffer is too small', () => {
      const image = nativeImage.createEmpty();

      image.addRepresentation({
        buffer: Buffer.from([1, 2, 3, 4]),
        width: 100,
        height: 100
      });

      expect(image.isEmpty()).toBe(true);
    });

    it('supports adding a buffer representation for a scale factor', () => {
      const image = nativeImage.createEmpty();

      const imageDataOne = image1x1;
      image.addRepresentation({
        scaleFactor: 1.0,
        buffer: nativeImage.createFromPath(imageDataOne.path).toPNG()
      });

      expect(image.getScaleFactors()).toEqual([1]);

      const imageDataTwo = image2x2;
      image.addRepresentation({
        scaleFactor: 2.0,
        buffer: nativeImage.createFromPath(imageDataTwo.path).toPNG()
      });

      expect(image.getScaleFactors()).toEqual([1, 2]);

      const imageDataThree = image3x3;
      image.addRepresentation({
        scaleFactor: 3.0,
        buffer: nativeImage.createFromPath(imageDataThree.path).toPNG()
      });

      expect(image.getScaleFactors()).toEqual([1, 2, 3]);

      image.addRepresentation({
        scaleFactor: 4.0,
        buffer: 'invalid' as any
      });

      // this one failed, so it shouldn't show up in the scale factors
      expect(image.getScaleFactors()).toEqual([1, 2, 3]);

      expect(image.isEmpty()).toBe(false);
      expect(image.getSize()).toEqual({ width: 1, height: 1 });

      expect(image.toDataURL({ scaleFactor: 1.0 })).toBe(imageDataOne.dataUrl);
      expect(image.toDataURL({ scaleFactor: 2.0 })).toBe(imageDataTwo.dataUrl);
      expect(image.toDataURL({ scaleFactor: 3.0 })).toBe(imageDataThree.dataUrl);
      expect(image.toDataURL({ scaleFactor: 4.0 })).toBe(imageDataThree.dataUrl);
    });

    it('supports adding a data URL representation for a scale factor', () => {
      const image = nativeImage.createEmpty();

      const imageDataOne = image1x1;
      image.addRepresentation({
        scaleFactor: 1.0,
        dataURL: imageDataOne.dataUrl
      });

      const imageDataTwo = image2x2;
      image.addRepresentation({
        scaleFactor: 2.0,
        dataURL: imageDataTwo.dataUrl
      });

      const imageDataThree = image3x3;
      image.addRepresentation({
        scaleFactor: 3.0,
        dataURL: imageDataThree.dataUrl
      });

      image.addRepresentation({
        scaleFactor: 4.0,
        dataURL: 'invalid'
      });

      expect(image.isEmpty()).toBe(false);
      expect(image.getSize()).toEqual({ width: 1, height: 1 });

      expect(image.toDataURL({ scaleFactor: 1.0 })).toBe(imageDataOne.dataUrl);
      expect(image.toDataURL({ scaleFactor: 2.0 })).toBe(imageDataTwo.dataUrl);
      expect(image.toDataURL({ scaleFactor: 3.0 })).toBe(imageDataThree.dataUrl);
      expect(image.toDataURL({ scaleFactor: 4.0 })).toBe(imageDataThree.dataUrl);
    });

    it('supports adding a representation to an existing image', () => {
      const imageDataOne = image1x1;
      const image = nativeImage.createFromPath(imageDataOne.path);

      const imageDataTwo = image2x2;
      image.addRepresentation({
        scaleFactor: 2.0,
        dataURL: imageDataTwo.dataUrl
      });

      const imageDataThree = image3x3;
      image.addRepresentation({
        scaleFactor: 2.0,
        dataURL: imageDataThree.dataUrl
      });

      expect(image.toDataURL({ scaleFactor: 1.0 })).toBe(imageDataOne.dataUrl);
      expect(image.toDataURL({ scaleFactor: 2.0 })).toBe(imageDataTwo.dataUrl);
    });
  });

  describe('in a context without a Node.js environment', () => {
    afterEach(closeAllWindows);

    const calls = ['toPNG()', 'toJPEG(100)', 'toBitmap()', 'getBitmap()'];
    if (process.platform === 'darwin') calls.push('getNativeHandle()');

    for (const call of calls) {
      it(`${call} throws instead of crashing the renderer`, async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            sandbox: true,
            contextIsolation: false,
            preload: path.join(fixturesPath, 'module', 'preload-electron.js')
          }
        });
        await w.loadURL('about:blank');

        const codes = await w.webContents.executeJavaScript(`
          const { nativeImage } = window.electron;
          [
            nativeImage.createEmpty(),
            nativeImage.createFromBitmap(new Uint8Array(4), { width: 1, height: 1 })
          ].map((image) => {
            try {
              image.${call};
              return 'did not throw';
            } catch (error) {
              return error.code;
            }
          });
        `);

        const code = 'ERR_BUFFER_CONTEXT_NOT_AVAILABLE';
        expect(codes).toEqual([code, code]);
      });
    }
  });
});
