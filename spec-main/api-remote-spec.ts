import * as path from 'path'
import { expect } from 'chai'
import { closeWindow } from './window-helpers'

import { ipcMain, BrowserWindow, nativeImage } from 'electron'
import { serialize, deserialize } from '../lib/common/type-utils';

describe('remote module', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  let w = null as unknown as BrowserWindow
  beforeEach(async () => {
    w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    await w.loadURL('about:blank')
  })
  afterEach(async () => {
    await closeWindow(w)
  })

  async function remotely(script: string) {
    // executeJavaScript never returns if the script throws an error, so catch
    // any errors manually.
    const assembledScript = `(function() {
      try {
        return { result: ${script} }
      } catch (e) {
        return { error: e.message }
      }
    })()`
    const {result, error} = await w.webContents.executeJavaScript(assembledScript)
    if (error) {
      throw new Error(error)
    }
    return result
  }

  describe('typeUtils serialization/deserialization', () => {
    it('serializes and deserializes an empty NativeImage', () => {
      const image = nativeImage.createEmpty();
      const serializedImage = serialize(image);
      const empty = deserialize(serializedImage);

      expect(empty.isEmpty()).to.be.true();
      expect(empty.getAspectRatio()).to.equal(1);
      expect(empty.toDataURL()).to.equal('data:image/png;base64,');
      expect(empty.toDataURL({ scaleFactor: 2.0 })).to.equal('data:image/png;base64,');
      expect(empty.getSize()).to.deep.equal({ width: 0, height: 0 });
      expect(empty.getBitmap()).to.be.empty();
      expect(empty.getBitmap({ scaleFactor: 2.0 })).to.be.empty();
      expect(empty.toBitmap()).to.be.empty();
      expect(empty.toBitmap({ scaleFactor: 2.0 })).to.be.empty();
      expect(empty.toJPEG(100)).to.be.empty();
      expect(empty.toPNG()).to.be.empty();
      expect(empty.toPNG({ scaleFactor: 2.0 })).to.be.empty();
    });

    it('serializes and deserializes a non-empty NativeImage', () => {
      const dataURL = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAAFklEQVQYlWP8//8/AwMDEwMDAwMDAwAkBgMBBMzldwAAAABJRU5ErkJggg==';
      const image = nativeImage.createFromDataURL(dataURL);
      const serializedImage = serialize(image);
      const nonEmpty = deserialize(serializedImage);

      expect(nonEmpty.isEmpty()).to.be.false();
      expect(nonEmpty.getAspectRatio()).to.equal(1);
      expect(nonEmpty.toDataURL()).to.not.be.empty();
      expect(nonEmpty.toDataURL({ scaleFactor: 1.0 })).to.equal(dataURL);
      expect(nonEmpty.getSize()).to.deep.equal({ width: 2, height: 2 });
      expect(nonEmpty.getBitmap()).to.not.be.empty();
      expect(nonEmpty.toPNG()).to.not.be.empty();
    });

    it('serializes and deserializes a non-empty NativeImage with multiple representations', () => {
      const image = nativeImage.createEmpty();

      const dataURL1 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAAC0lEQVQYlWNgAAIAAAUAAdafFs0AAAAASUVORK5CYII=';
      image.addRepresentation({ scaleFactor: 1.0, dataURL: dataURL1 });

      const dataURL2 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAAFklEQVQYlWP8//8/AwMDEwMDAwMDAwAkBgMBBMzldwAAAABJRU5ErkJggg==';
      image.addRepresentation({ scaleFactor: 2.0, dataURL: dataURL2 });

      const serializedImage = serialize(image);
      const nonEmpty = deserialize(serializedImage);

      expect(nonEmpty.isEmpty()).to.be.false();
      expect(nonEmpty.getAspectRatio()).to.equal(1);
      expect(nonEmpty.getSize()).to.deep.equal({ width: 1, height: 1 });
      expect(nonEmpty.getBitmap()).to.not.be.empty();
      expect(nonEmpty.getBitmap({ scaleFactor: 1.0 })).to.not.be.empty();
      expect(nonEmpty.getBitmap({ scaleFactor: 2.0 })).to.not.be.empty();
      expect(nonEmpty.toBitmap()).to.not.be.empty();
      expect(nonEmpty.toBitmap({ scaleFactor: 1.0 })).to.not.be.empty();
      expect(nonEmpty.toBitmap({ scaleFactor: 2.0 })).to.not.be.empty();
      expect(nonEmpty.toPNG()).to.not.be.empty();
      expect(nonEmpty.toPNG({ scaleFactor: 1.0 })).to.not.be.empty();
      expect(nonEmpty.toPNG({ scaleFactor: 2.0 })).to.not.be.empty();
      expect(nonEmpty.toDataURL()).to.not.be.empty();
      expect(nonEmpty.toDataURL({ scaleFactor: 1.0 })).to.equal(dataURL1);
      expect(nonEmpty.toDataURL({ scaleFactor: 2.0 })).to.equal(dataURL2);
    });

    it('serializes and deserializes an Array', () => {
      const array = [1, 2, 3, 4, 5];
      const serialized = serialize(array);
      const deserialized = deserialize(serialized);

      expect(deserialized).to.deep.equal(array);
    });

    it('serializes and deserializes a Buffer', () => {
      const buffer = Buffer.from('hello world!', 'utf-8');
      const serialized = serialize(buffer);
      const deserialized = deserialize(serialized);

      expect(deserialized).to.deep.equal(buffer);
    });

    it('serializes and deserializes a Boolean', () => {
      const bool = true;
      const serialized = serialize(bool);
      const deserialized = deserialize(serialized);

      expect(deserialized).to.equal(bool);
    });

    it('serializes and deserializes a Number', () => {
      const number = 42;
      const serialized = serialize(number);
      const deserialized = deserialize(serialized);

      expect(deserialized).to.equal(number);
    });

    it('serializes and deserializes a String', () => {
      const str = 'hello world';
      const serialized = serialize(str);
      const deserialized = deserialize(serialized);

      expect(deserialized).to.equal(str);
    });

    it('serializes and deserializes a simple Object', () => {
      const obj = { hello: 'world', 'answer-to-everything': 42 };
      const serialized = serialize(obj);
      const deserialized = deserialize(serialized);

      expect(deserialized).to.deep.equal(obj);
    });
  });

  describe('remote.getGlobal filtering', () => {
    it('can return custom values', async () => {
      w.webContents.once('remote-get-global', (event, name) => {
        event.returnValue = name
      })
      expect(await remotely(`require('electron').remote.getGlobal('test')`)).to.equal('test')
    })

    it('throws when no returnValue set', async () => {
      w.webContents.once('remote-get-global', (event, name) => {
        event.preventDefault()
      })
      await expect(remotely(`require('electron').remote.getGlobal('test')`)).to.eventually.be.rejected(`Blocked remote.getGlobal('test')`)
    })
  })

  describe('remote.getBuiltin filtering', () => {
    it('can return custom values', async () => {
      w.webContents.once('remote-get-builtin', (event, name) => {
        event.returnValue = name
      })
      expect(await remotely(`require('electron').remote.getBuiltin('test')`)).to.equal('test')
    })

    it('throws when no returnValue set', async () => {
      w.webContents.once('remote-get-builtin', (event, name) => {
        event.preventDefault()
      })
      await expect(remotely(`require('electron').remote.getBuiltin('test')`)).to.eventually.be.rejected(`Blocked remote.getGlobal('test')`)
    })
  })

  describe('remote.require filtering', () => {
    it('can return custom values', async () => {
      w.webContents.once('remote-require', (event, name) => {
        event.returnValue = name
      })
      expect(await remotely(`require('electron').remote.require('test')`)).to.equal('test')
    })

    it('throws when no returnValue set', async () => {
      w.webContents.once('remote-require', (event, name) => {
        event.preventDefault()
      })
      await expect(remotely(`require('electron').remote.require('test')`)).to.eventually.be.rejected(`Blocked remote.require('test')`)
    })
  })

  describe('remote.getCurrentWindow filtering', () => {
    it('can return custom value', async () => {
      w.webContents.once('remote-get-current-window', (e) => {
        e.returnValue = 'some window'
      })
      expect(await remotely(`require('electron').remote.getCurrentWindow()`)).to.equal('some window')
    })

    it('throws when no returnValue set', async () => {
      w.webContents.once('remote-get-current-window', (event) => {
        event.preventDefault()
      })
      await expect(remotely(`require('electron').remote.getCurrentWindow()`)).to.eventually.be.rejected(`Blocked remote.getCurrentWindow()`)
    })
  })

  describe('remote.getCurrentWebContents filtering', () => {
    it('can return custom value', async () => {
      w.webContents.once('remote-get-current-web-contents', (event) => {
        event.returnValue = 'some web contents'
      })
      expect(await remotely(`require('electron').remote.getCurrentWebContents()`)).to.equal('some web contents')
    })

    it('throws when no returnValue set', async () => {
      w.webContents.once('remote-get-current-web-contents', (event) => {
        event.preventDefault()
      })
      await expect(remotely(`require('electron').remote.getCurrentWebContents()`)).to.eventually.be.rejected(`Blocked remote.getCurrentWebContents()`)
    })
  })

  describe('remote references', () => {
    it('render-view-deleted is sent when page is destroyed', (done) => {
      w.webContents.once('render-view-deleted' as any, () => {
        done()
      })
      w.destroy()
    })

    // The ELECTRON_BROWSER_CONTEXT_RELEASE message relies on this to work.
    it('message can be sent on exit when page is being navigated', (done) => {
      after(() => { ipcMain.removeAllListeners('SENT_ON_EXIT') })
      ipcMain.once('SENT_ON_EXIT', () => {
        done()
      })
      w.webContents.once('did-finish-load', () => {
        w.webContents.loadURL('about:blank')
      })
      w.loadFile(path.join(fixtures, 'api', 'send-on-exit.html'))
    })
  })
})
