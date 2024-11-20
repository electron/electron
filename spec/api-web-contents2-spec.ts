import { BrowserWindow, ipcMain, webContents, session } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as http from 'node:http';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, defer, listen, ifit } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('webContents2 module', () => {
  describe('getFocusedWebContents() API', () => {
    afterEach(closeAllWindows);

    // FIXME
    ifit(!(process.platform === 'win32' && process.arch === 'arm64'))('returns the focused web contents', async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadFile(path.join(__dirname, 'fixtures', 'blank.html'));
      expect(webContents.getFocusedWebContents()?.id).to.equal(w.webContents.id);

      const devToolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools();
      await devToolsOpened;
      expect(webContents.getFocusedWebContents()?.id).to.equal(w.webContents.devToolsWebContents!.id);
      const devToolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devToolsClosed;
      expect(webContents.getFocusedWebContents()?.id).to.equal(w.webContents.id);
    });

    it('does not crash when called on a detached dev tools window', async () => {
      const w = new BrowserWindow({ show: true });

      w.webContents.openDevTools({ mode: 'detach' });
      w.webContents.inspectElement(100, 100);

      // For some reason we have to wait for two focused events...?
      await once(w.webContents, 'devtools-focused');

      expect(() => { webContents.getFocusedWebContents(); }).to.not.throw();

      // Work around https://github.com/electron/electron/issues/19985
      await setTimeout();

      const devToolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devToolsClosed;
      expect(() => { webContents.getFocusedWebContents(); }).to.not.throw();
    });
  });

  describe('setDevToolsWebContents() API', () => {
    afterEach(closeAllWindows);
    it('sets arbitrary webContents as devtools', async () => {
      const w = new BrowserWindow({ show: false });
      const devtools = new BrowserWindow({ show: false });
      const promise = once(devtools.webContents, 'dom-ready');
      w.webContents.setDevToolsWebContents(devtools.webContents);
      w.webContents.openDevTools();
      await promise;
      expect(devtools.webContents.getURL().startsWith('devtools://devtools')).to.be.true();
      const result = await devtools.webContents.executeJavaScript('InspectorFrontendHost.constructor.name');
      expect(result).to.equal('InspectorFrontendHostImpl');
      devtools.destroy();
    });
  });

  describe('isFocused() API', () => {
    it('returns false when the window is hidden', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      expect(w.isVisible()).to.be.false();
      expect(w.webContents.isFocused()).to.be.false();
    });
  });

  describe('isCurrentlyAudible() API', () => {
    afterEach(closeAllWindows);
    it('returns whether audio is playing', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      await w.webContents.executeJavaScript(`
        window.context = new AudioContext
        // Start in suspended state, because of the
        // new web audio api policy.
        context.suspend()
        window.oscillator = context.createOscillator()
        oscillator.connect(context.destination)
        oscillator.start()
      `);
      let p = once(w.webContents, 'audio-state-changed');
      w.webContents.executeJavaScript('context.resume()');
      await p;
      expect(w.webContents.isCurrentlyAudible()).to.be.true();
      p = once(w.webContents, 'audio-state-changed');
      w.webContents.executeJavaScript('oscillator.stop()');
      await p;
      expect(w.webContents.isCurrentlyAudible()).to.be.false();
    });
  });

  describe('openDevTools() API', () => {
    afterEach(closeAllWindows);
    it('can show window with activation', async () => {
      const w = new BrowserWindow({ show: false });
      const focused = once(w, 'focus');
      w.show();
      await focused;
      expect(w.isFocused()).to.be.true();
      const blurred = once(w, 'blur');
      w.webContents.openDevTools({ mode: 'detach', activate: true });
      await Promise.all([
        once(w.webContents, 'devtools-opened'),
        once(w.webContents, 'devtools-focused')
      ]);
      await blurred;
      expect(w.isFocused()).to.be.false();
    });

    it('can show window without activation', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false });
      await devtoolsOpened;
      expect(w.webContents.isDevToolsOpened()).to.be.true();
    });

    it('can show a DevTools window with custom title', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false, title: 'myTitle' });
      await devtoolsOpened;
      expect(w.webContents.getDevToolsTitle()).to.equal('myTitle');
    });

    it('can re-open devtools', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: true });
      await devtoolsOpened;
      expect(w.webContents.isDevToolsOpened()).to.be.true();

      const devtoolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devtoolsClosed;
      expect(w.webContents.isDevToolsOpened()).to.be.false();

      const devtoolsOpened2 = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: true });
      await devtoolsOpened2;
      expect(w.webContents.isDevToolsOpened()).to.be.true();
    });
  });

  describe('setDevToolsTitle() API', () => {
    afterEach(closeAllWindows);
    it('can set devtools title with function', async () => {
      const w = new BrowserWindow({ show: false });
      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.openDevTools({ mode: 'detach', activate: false });
      await devtoolsOpened;
      expect(w.webContents.isDevToolsOpened()).to.be.true();
      w.webContents.setDevToolsTitle('newTitle');
      expect(w.webContents.getDevToolsTitle()).to.equal('newTitle');
    });
  });

  describe('before-input-event event', () => {
    afterEach(closeAllWindows);
    it('can prevent document keyboard events', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadFile(path.join(fixturesPath, 'pages', 'key-events.html'));
      const keyDown = new Promise(resolve => {
        ipcMain.once('keydown', (event, key) => resolve(key));
      });
      w.webContents.once('before-input-event', (event, input) => {
        if (input.key === 'a') event.preventDefault();
      });
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'a' });
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'b' });
      expect(await keyDown).to.equal('b');
    });

    it('has the correct properties', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));
      const testBeforeInput = async (opts: any) => {
        const modifiers = [];
        if (opts.shift) modifiers.push('shift');
        if (opts.control) modifiers.push('control');
        if (opts.alt) modifiers.push('alt');
        if (opts.meta) modifiers.push('meta');
        if (opts.isAutoRepeat) modifiers.push('isAutoRepeat');

        const p = once(w.webContents, 'before-input-event') as Promise<[any, Electron.Input]>;
        w.webContents.sendInputEvent({
          type: opts.type,
          keyCode: opts.keyCode,
          modifiers: modifiers as any
        });
        const [, input] = await p;

        expect(input.type).to.equal(opts.type);
        expect(input.key).to.equal(opts.key);
        expect(input.code).to.equal(opts.code);
        expect(input.isAutoRepeat).to.equal(opts.isAutoRepeat);
        expect(input.shift).to.equal(opts.shift);
        expect(input.control).to.equal(opts.control);
        expect(input.alt).to.equal(opts.alt);
        expect(input.meta).to.equal(opts.meta);
      };
      await testBeforeInput({
        type: 'keyDown',
        key: 'A',
        code: 'KeyA',
        keyCode: 'a',
        shift: true,
        control: true,
        alt: true,
        meta: true,
        isAutoRepeat: true
      });
      await testBeforeInput({
        type: 'keyUp',
        key: '.',
        code: 'Period',
        keyCode: '.',
        shift: false,
        control: true,
        alt: true,
        meta: false,
        isAutoRepeat: false
      });
      await testBeforeInput({
        type: 'keyUp',
        key: '!',
        code: 'Digit1',
        keyCode: '1',
        shift: true,
        control: false,
        alt: false,
        meta: true,
        isAutoRepeat: false
      });
      await testBeforeInput({
        type: 'keyUp',
        key: 'Tab',
        code: 'Tab',
        keyCode: 'Tab',
        shift: false,
        control: true,
        alt: false,
        meta: false,
        isAutoRepeat: true
      });
    });
  });

  // On Mac, zooming isn't done with the mouse wheel.
  ifdescribe(process.platform !== 'darwin')('zoom-changed', () => {
    afterEach(closeAllWindows);
    it('is emitted with the correct zoom-in info', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));

      const testZoomChanged = async () => {
        w.webContents.sendInputEvent({
          type: 'mouseWheel',
          x: 300,
          y: 300,
          deltaX: 0,
          deltaY: 1,
          wheelTicksX: 0,
          wheelTicksY: 1,
          modifiers: ['control', 'meta']
        });

        const [, zoomDirection] = await once(w.webContents, 'zoom-changed') as [any, string];
        expect(zoomDirection).to.equal('in');
      };

      await testZoomChanged();
    });

    it('is emitted with the correct zoom-out info', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));

      const testZoomChanged = async () => {
        w.webContents.sendInputEvent({
          type: 'mouseWheel',
          x: 300,
          y: 300,
          deltaX: 0,
          deltaY: -1,
          wheelTicksX: 0,
          wheelTicksY: -1,
          modifiers: ['control', 'meta']
        });

        const [, zoomDirection] = await once(w.webContents, 'zoom-changed') as [any, string];
        expect(zoomDirection).to.equal('out');
      };

      await testZoomChanged();
    });
  });

  describe('sendInputEvent(event)', () => {
    let w: BrowserWindow;
    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      await w.loadFile(path.join(fixturesPath, 'pages', 'key-events.html'));
    });
    afterEach(closeAllWindows);

    it('can send keydown events', async () => {
      const keydown = once(ipcMain, 'keydown');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'A' });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keydown;
      expect(key).to.equal('a');
      expect(code).to.equal('KeyA');
      expect(keyCode).to.equal(65);
      expect(shiftKey).to.be.false();
      expect(ctrlKey).to.be.false();
      expect(altKey).to.be.false();
    });

    it('can send keydown events with modifiers', async () => {
      const keydown = once(ipcMain, 'keydown');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Z', modifiers: ['shift', 'ctrl'] });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keydown;
      expect(key).to.equal('Z');
      expect(code).to.equal('KeyZ');
      expect(keyCode).to.equal(90);
      expect(shiftKey).to.be.true();
      expect(ctrlKey).to.be.true();
      expect(altKey).to.be.false();
    });

    it('can send keydown events with special keys', async () => {
      const keydown = once(ipcMain, 'keydown');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Tab', modifiers: ['alt'] });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keydown;
      expect(key).to.equal('Tab');
      expect(code).to.equal('Tab');
      expect(keyCode).to.equal(9);
      expect(shiftKey).to.be.false();
      expect(ctrlKey).to.be.false();
      expect(altKey).to.be.true();
    });

    it('can send char events', async () => {
      const keypress = once(ipcMain, 'keypress');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'A' });
      w.webContents.sendInputEvent({ type: 'char', keyCode: 'A' });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keypress;
      expect(key).to.equal('a');
      expect(code).to.equal('KeyA');
      expect(keyCode).to.equal(65);
      expect(shiftKey).to.be.false();
      expect(ctrlKey).to.be.false();
      expect(altKey).to.be.false();
    });

    it('can correctly convert accelerators to key codes', async () => {
      const keyup = once(ipcMain, 'keyup');
      w.webContents.sendInputEvent({ keyCode: 'Plus', type: 'char' });
      w.webContents.sendInputEvent({ keyCode: 'Space', type: 'char' });
      w.webContents.sendInputEvent({ keyCode: 'Plus', type: 'char' });
      w.webContents.sendInputEvent({ keyCode: 'Space', type: 'char' });
      w.webContents.sendInputEvent({ keyCode: 'Plus', type: 'char' });
      w.webContents.sendInputEvent({ keyCode: 'Plus', type: 'keyUp' });

      await keyup;
      const inputText = await w.webContents.executeJavaScript('document.getElementById("input").value');
      expect(inputText).to.equal('+ + +');
    });

    it('can send char events with modifiers', async () => {
      const keypress = once(ipcMain, 'keypress');
      w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'Z' });
      w.webContents.sendInputEvent({ type: 'char', keyCode: 'Z', modifiers: ['shift', 'ctrl'] });
      const [, key, code, keyCode, shiftKey, ctrlKey, altKey] = await keypress;
      expect(key).to.equal('Z');
      expect(code).to.equal('KeyZ');
      expect(keyCode).to.equal(90);
      expect(shiftKey).to.be.true();
      expect(ctrlKey).to.be.true();
      expect(altKey).to.be.false();
    });
  });

  describe('insertCSS', () => {
    afterEach(closeAllWindows);
    it('supports inserting CSS', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      await w.webContents.insertCSS('body { background-repeat: round; }');
      const result = await w.webContents.executeJavaScript('window.getComputedStyle(document.body).getPropertyValue("background-repeat")');
      expect(result).to.equal('round');
    });

    it('supports removing inserted CSS', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      const key = await w.webContents.insertCSS('body { background-repeat: round; }');
      await w.webContents.removeInsertedCSS(key);
      const result = await w.webContents.executeJavaScript('window.getComputedStyle(document.body).getPropertyValue("background-repeat")');
      expect(result).to.equal('repeat');
    });
  });

  describe('inspectElement()', () => {
    afterEach(closeAllWindows);
    it('supports inspecting an element in the devtools', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      const event = once(w.webContents, 'devtools-opened');
      w.webContents.inspectElement(10, 10);
      await event;
    });
  });

  describe('startDrag({file, icon})', () => {
    it('throws errors for a missing file or a missing/empty icon', () => {
      const w = new BrowserWindow({ show: false });
      expect(() => {
        w.webContents.startDrag({ icon: path.join(fixturesPath, 'assets', 'logo.png') } as any);
      }).to.throw('Must specify either \'file\' or \'files\' option');

      expect(() => {
        w.webContents.startDrag({ file: __filename } as any);
      }).to.throw('\'icon\' parameter is required');

      expect(() => {
        w.webContents.startDrag({ file: __filename, icon: path.join(fixturesPath, 'blank.png') });
      }).to.throw(/Failed to load image from path (.+)/);
    });
  });

  describe('focus APIs', () => {
    describe('focus()', () => {
      afterEach(closeAllWindows);
      it('does not blur the focused window when the web contents is hidden', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
        w.show();
        await w.loadURL('about:blank');
        w.focus();
        const child = new BrowserWindow({ show: false });
        child.loadURL('about:blank');
        child.webContents.focus();
        const currentFocused = w.isFocused();
        const childFocused = child.isFocused();
        child.close();
        expect(currentFocused).to.be.true();
        expect(childFocused).to.be.false();
      });

      it('does not crash when focusing a WebView webContents', async () => {
        const w = new BrowserWindow({
          show: false,
          webPreferences: {
            nodeIntegration: true,
            webviewTag: true
          }
        });

        w.show();
        await w.loadURL('data:text/html,<webview src="data:text/html,hi"></webview>');

        const wc = webContents.getAllWebContents().find((wc) => wc.getType() === 'webview')!;
        expect(() => wc.focus()).to.not.throw();
      });
    });

    const moveFocusToDevTools = async (win: BrowserWindow) => {
      const devToolsOpened = once(win.webContents, 'devtools-opened');
      win.webContents.openDevTools({ mode: 'right' });
      await devToolsOpened;
      win.webContents.devToolsWebContents!.focus();
    };

    describe('focus event', () => {
      afterEach(closeAllWindows);

      it('is triggered when web contents is focused', async () => {
        const w = new BrowserWindow({ show: false });
        await w.loadURL('about:blank');
        await moveFocusToDevTools(w);
        const focusPromise = once(w.webContents, 'focus');
        w.webContents.focus();
        await expect(focusPromise).to.eventually.be.fulfilled();
      });
    });

    describe('blur event', () => {
      afterEach(closeAllWindows);
      it('is triggered when web contents is blurred', async () => {
        const w = new BrowserWindow({ show: true });
        await w.loadURL('about:blank');
        w.webContents.focus();
        const blurPromise = once(w.webContents, 'blur');
        await moveFocusToDevTools(w);
        await expect(blurPromise).to.eventually.be.fulfilled();
      });
    });
  });

  describe('getOSProcessId()', () => {
    afterEach(closeAllWindows);
    it('returns a valid process id', async () => {
      const w = new BrowserWindow({ show: false });
      expect(w.webContents.getOSProcessId()).to.equal(0);

      await w.loadURL('about:blank');
      expect(w.webContents.getOSProcessId()).to.be.above(0);
    });
  });

  describe('getMediaSourceId()', () => {
    afterEach(closeAllWindows);
    it('returns a valid stream id', () => {
      const w = new BrowserWindow({ show: false });
      expect(w.webContents.getMediaSourceId(w.webContents)).to.be.a('string').that.is.not.empty();
    });
  });

  describe('userAgent APIs', () => {
    it('is not empty by default', () => {
      const w = new BrowserWindow({ show: false });
      const userAgent = w.webContents.getUserAgent();
      expect(userAgent).to.be.a('string').that.is.not.empty();
    });

    it('can set the user agent (functions)', () => {
      const w = new BrowserWindow({ show: false });
      const userAgent = w.webContents.getUserAgent();

      w.webContents.setUserAgent('my-user-agent');
      expect(w.webContents.getUserAgent()).to.equal('my-user-agent');

      w.webContents.setUserAgent(userAgent);
      expect(w.webContents.getUserAgent()).to.equal(userAgent);
    });

    it('can set the user agent (properties)', () => {
      const w = new BrowserWindow({ show: false });
      const userAgent = w.webContents.userAgent;

      w.webContents.userAgent = 'my-user-agent';
      expect(w.webContents.userAgent).to.equal('my-user-agent');

      w.webContents.userAgent = userAgent;
      expect(w.webContents.userAgent).to.equal(userAgent);
    });
  });

  describe('audioMuted APIs', () => {
    it('can set the audio mute level (functions)', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.setAudioMuted(true);
      expect(w.webContents.isAudioMuted()).to.be.true();

      w.webContents.setAudioMuted(false);
      expect(w.webContents.isAudioMuted()).to.be.false();
    });

    it('can set the audio mute level (functions)', () => {
      const w = new BrowserWindow({ show: false });

      w.webContents.audioMuted = true;
      expect(w.webContents.audioMuted).to.be.true();

      w.webContents.audioMuted = false;
      expect(w.webContents.audioMuted).to.be.false();
    });
  });

  describe('zoom api', () => {
    const hostZoomMap: Record<string, number> = {
      host1: 0.3,
      host2: 0.7,
      host3: 0.2
    };

    before(() => {
      const protocol = session.defaultSession.protocol;
      protocol.registerStringProtocol(standardScheme, (request, callback) => {
        const response = `<script>
                            const {ipcRenderer} = require('electron')
                            ipcRenderer.send('set-zoom', window.location.hostname)
                            ipcRenderer.on(window.location.hostname + '-zoom-set', () => {
                              ipcRenderer.send(window.location.hostname + '-zoom-level')
                            })
                          </script>`;
        callback({ data: response, mimeType: 'text/html' });
      });
    });

    after(() => {
      const protocol = session.defaultSession.protocol;
      protocol.unregisterProtocol(standardScheme);
    });

    afterEach(closeAllWindows);

    it('throws on an invalid zoomFactor', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');

      expect(() => {
        w.webContents.setZoomFactor(0.0);
      }).to.throw(/'zoomFactor' must be a double greater than 0.0/);

      expect(() => {
        w.webContents.setZoomFactor(-2.0);
      }).to.throw(/'zoomFactor' must be a double greater than 0.0/);
    });

    it('can set the correct zoom level (functions)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomLevel = w.webContents.getZoomLevel();
        expect(zoomLevel).to.eql(0.0);
        w.webContents.setZoomLevel(0.5);
        const newZoomLevel = w.webContents.getZoomLevel();
        expect(newZoomLevel).to.eql(0.5);
      } finally {
        w.webContents.setZoomLevel(0);
      }
    });

    it('can set the correct zoom level (properties)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomLevel = w.webContents.zoomLevel;
        expect(zoomLevel).to.eql(0.0);
        w.webContents.zoomLevel = 0.5;
        const newZoomLevel = w.webContents.zoomLevel;
        expect(newZoomLevel).to.eql(0.5);
      } finally {
        w.webContents.zoomLevel = 0;
      }
    });

    it('can set the correct zoom factor (functions)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomFactor = w.webContents.getZoomFactor();
        expect(zoomFactor).to.eql(1.0);

        w.webContents.setZoomFactor(0.5);
        const newZoomFactor = w.webContents.getZoomFactor();
        expect(newZoomFactor).to.eql(0.5);
      } finally {
        w.webContents.setZoomFactor(1.0);
      }
    });

    it('can set the correct zoom factor (properties)', async () => {
      const w = new BrowserWindow({ show: false });
      try {
        await w.loadURL('about:blank');
        const zoomFactor = w.webContents.zoomFactor;
        expect(zoomFactor).to.eql(1.0);

        w.webContents.zoomFactor = 0.5;
        const newZoomFactor = w.webContents.zoomFactor;
        expect(newZoomFactor).to.eql(0.5);
      } finally {
        w.webContents.zoomFactor = 1.0;
      }
    });

    it('can persist zoom level across navigation', (done) => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      let finalNavigation = false;
      ipcMain.on('set-zoom', (e, host) => {
        const zoomLevel = hostZoomMap[host];
        if (!finalNavigation) w.webContents.zoomLevel = zoomLevel;
        e.sender.send(`${host}-zoom-set`);
      });
      ipcMain.on('host1-zoom-level', (e) => {
        try {
          const zoomLevel = e.sender.getZoomLevel();
          const expectedZoomLevel = hostZoomMap.host1;
          expect(zoomLevel).to.equal(expectedZoomLevel);
          if (finalNavigation) {
            done();
          } else {
            w.loadURL(`${standardScheme}://host2`);
          }
        } catch (e) {
          done(e);
        }
      });
      ipcMain.once('host2-zoom-level', (e) => {
        try {
          const zoomLevel = e.sender.getZoomLevel();
          const expectedZoomLevel = hostZoomMap.host2;
          expect(zoomLevel).to.equal(expectedZoomLevel);
          finalNavigation = true;
          w.webContents.goBack();
        } catch (e) {
          done(e);
        }
      });
      w.loadURL(`${standardScheme}://host1`);
    });

    it('can propagate zoom level across same session', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
      const w2 = new BrowserWindow({ show: false });

      defer(() => {
        w2.setClosable(true);
        w2.close();
      });

      await w.loadURL(`${standardScheme}://host3`);
      w.webContents.zoomLevel = hostZoomMap.host3;

      await w2.loadURL(`${standardScheme}://host3`);
      const zoomLevel1 = w.webContents.zoomLevel;
      expect(zoomLevel1).to.equal(hostZoomMap.host3);

      const zoomLevel2 = w2.webContents.zoomLevel;
      expect(zoomLevel1).to.equal(zoomLevel2);
    });

    it('cannot propagate zoom level across different session', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true } });
      const w2 = new BrowserWindow({
        show: false,
        webPreferences: {
          partition: 'temp'
        }
      });
      const protocol = w2.webContents.session.protocol;
      protocol.registerStringProtocol(standardScheme, (request, callback) => {
        callback('hello');
      });

      defer(() => {
        w2.setClosable(true);
        w2.close();

        protocol.unregisterProtocol(standardScheme);
      });

      await w.loadURL(`${standardScheme}://host3`);
      w.webContents.zoomLevel = hostZoomMap.host3;

      await w2.loadURL(`${standardScheme}://host3`);
      const zoomLevel1 = w.webContents.zoomLevel;
      expect(zoomLevel1).to.equal(hostZoomMap.host3);

      const zoomLevel2 = w2.webContents.zoomLevel;
      expect(zoomLevel2).to.equal(0);
      expect(zoomLevel1).to.not.equal(zoomLevel2);
    });

    it('can persist when it contains iframe', (done) => {
      const w = new BrowserWindow({ show: false });
      let server = http.createServer((req, res) => {
        setTimeout(200).then(() => {
          res.end();
        });
      });
      defer(() => {
        server.close();
        server = null as unknown as http.Server;
      });
      listen(server).then(({ url }) => {
        const content = `<iframe src=${url}></iframe>`;
        w.webContents.on('did-frame-finish-load', (e, isMainFrame) => {
          if (!isMainFrame) {
            try {
              const zoomLevel = w.webContents.zoomLevel;
              expect(zoomLevel).to.equal(2.0);

              w.webContents.zoomLevel = 0;
              done();
            } catch (e) {
              done(e);
            }
          }
        });
        w.webContents.on('dom-ready', () => {
          w.webContents.zoomLevel = 2.0;
        });
        w.loadURL(`data:text/html,${content}`);
      });
    });

    it('cannot propagate when used with webframe', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      const w2 = new BrowserWindow({ show: false });

      const temporaryZoomSet = once(ipcMain, 'temporary-zoom-set');
      w.loadFile(path.join(fixturesPath, 'pages', 'webframe-zoom.html'));
      await temporaryZoomSet;

      const finalZoomLevel = w.webContents.getZoomLevel();
      await w2.loadFile(path.join(fixturesPath, 'pages', 'c.html'));
      const zoomLevel1 = w.webContents.zoomLevel;
      const zoomLevel2 = w2.webContents.zoomLevel;

      w2.setClosable(true);
      w2.close();

      expect(zoomLevel1).to.equal(finalZoomLevel);
      expect(zoomLevel2).to.equal(0);
      expect(zoomLevel1).to.not.equal(zoomLevel2);
    });

    describe('with unique domains', () => {
      let server: http.Server;
      let serverUrl: string;
      let crossSiteUrl: string;

      before(async () => {
        server = http.createServer((req, res) => {
          setTimeout().then(() => res.end('hey'));
        });
        serverUrl = (await listen(server)).url;
        crossSiteUrl = serverUrl.replace('127.0.0.1', 'localhost');
      });

      after(() => {
        server.close();
        server = null as unknown as http.Server;
      });

      it('cannot persist zoom level after navigation with webFrame', async () => {
        const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
        const source = `
          const {ipcRenderer, webFrame} = require('electron')
          webFrame.setZoomLevel(0.6)
          ipcRenderer.send('zoom-level-set', webFrame.getZoomLevel())
        `;
        const zoomLevelPromise = once(ipcMain, 'zoom-level-set');
        await w.loadURL(serverUrl);
        await w.webContents.executeJavaScript(source);
        let [, zoomLevel] = await zoomLevelPromise;
        expect(zoomLevel).to.equal(0.6);
        const loadPromise = once(w.webContents, 'did-finish-load');
        await w.loadURL(crossSiteUrl);
        await loadPromise;
        zoomLevel = w.webContents.zoomLevel;
        expect(zoomLevel).to.equal(0);
      });
    });
  });
});
