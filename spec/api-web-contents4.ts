import { BrowserWindow, ipcMain, webContents, session, app, WebContents } from 'electron/main';

import { expect } from 'chai';

import * as cp from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import * as os from 'node:os';
import * as path from 'node:path';
import * as url from 'node:url';

import { ifdescribe, defer, listen } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');
const features = process._linkedBinding('electron_common_features');

describe('webContents4 module', () => {
  ifdescribe(features.isPrintingEnabled())('printToPDF()', () => {
    const readPDF = async (data: any) => {
      const tmpDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'e-spec-printtopdf-'));
      const pdfPath = path.resolve(tmpDir, 'test.pdf');
      await fs.promises.writeFile(pdfPath, data);
      const pdfReaderPath = path.resolve(fixturesPath, 'api', 'pdf-reader.mjs');

      const result = cp.spawn(process.execPath, [pdfReaderPath, pdfPath], {
        stdio: 'pipe'
      });

      const stdout: Buffer[] = [];
      const stderr: Buffer[] = [];
      result.stdout.on('data', (chunk) => stdout.push(chunk));
      result.stderr.on('data', (chunk) => stderr.push(chunk));

      const [code, signal] = await new Promise<[number | null, NodeJS.Signals | null]>((resolve) => {
        result.on('close', (code, signal) => {
          resolve([code, signal]);
        });
      });
      await fs.promises.rm(tmpDir, { force: true, recursive: true });
      if (code !== 0) {
        const errMsg = Buffer.concat(stderr).toString().trim();
        console.error(`Error parsing PDF file, exit code was ${code}; signal was ${signal}, error: ${errMsg}`);
      }
      return JSON.parse(Buffer.concat(stdout).toString().trim());
    };

    let w: BrowserWindow;

    const containsText = (items: any[], text: RegExp) => {
      return items.some(({ str }: { str: string }) => str.match(text));
    };

    beforeEach(() => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true
        }
      });
    });

    afterEach(closeAllWindows);

    it('rejects on incorrectly typed parameters', async () => {
      const badTypes = {
        landscape: [],
        displayHeaderFooter: '123',
        printBackground: 2,
        scale: 'not-a-number',
        pageSize: 'IAmAPageSize',
        margins: 'terrible',
        pageRanges: { oops: 'im-not-the-right-key' },
        headerTemplate: [1, 2, 3],
        footerTemplate: [4, 5, 6],
        preferCSSPageSize: 'no',
        generateTaggedPDF: 'wtf',
        generateDocumentOutline: [7, 8, 9]
      };

      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      // These will hard crash in Chromium unless we type-check
      for (const [key, value] of Object.entries(badTypes)) {
        const param = { [key]: value };
        await expect(w.webContents.printToPDF(param)).to.eventually.be.rejected();
      }
    });

    it('rejects when margins exceed physical page size', async () => {
      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      await expect(w.webContents.printToPDF({
        pageSize: 'Letter',
        margins: {
          top: 100,
          bottom: 100,
          left: 5,
          right: 5
        }
      })).to.eventually.be.rejectedWith('margins must be less than or equal to pageSize');
    });

    it('does not crash when called multiple times in parallel', async () => {
      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      const promises = [];
      for (let i = 0; i < 3; i++) {
        promises.push(w.webContents.printToPDF({}));
      }

      const results = await Promise.all(promises);
      for (const data of results) {
        expect(data).to.be.an.instanceof(Buffer).that.is.not.empty();
      }
    });

    it('does not crash when called multiple times in sequence', async () => {
      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      const results = [];
      for (let i = 0; i < 3; i++) {
        const result = await w.webContents.printToPDF({});
        results.push(result);
      }

      for (const data of results) {
        expect(data).to.be.an.instanceof(Buffer).that.is.not.empty();
      }
    });

    it('can print a PDF with default settings', async () => {
      await w.loadURL('data:text/html,<h1>Hello, World!</h1>');

      const data = await w.webContents.printToPDF({});
      expect(data).to.be.an.instanceof(Buffer).that.is.not.empty();
    });

    type PageSizeString = Exclude<Required<Electron.PrintToPDFOptions>['pageSize'], Electron.Size>;

    it('with custom page sizes', async () => {
      const paperFormats: Record<PageSizeString, ElectronInternal.PageSize> = {
        Letter: { width: 8.5, height: 11 },
        Legal: { width: 8.5, height: 14 },
        Tabloid: { width: 11, height: 17 },
        Ledger: { width: 17, height: 11 },
        A0: { width: 33.1, height: 46.8 },
        A1: { width: 23.4, height: 33.1 },
        A2: { width: 16.54, height: 23.4 },
        A3: { width: 11.7, height: 16.54 },
        A4: { width: 8.27, height: 11.7 },
        A5: { width: 5.83, height: 8.27 },
        A6: { width: 4.13, height: 5.83 }
      };

      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'print-to-pdf-small.html'));

      for (const format of Object.keys(paperFormats) as PageSizeString[]) {
        const data = await w.webContents.printToPDF({ pageSize: format });

        const pdfInfo = await readPDF(data);

        // page.view is [top, left, width, height].
        const width = pdfInfo.view[2] / 72;
        const height = pdfInfo.view[3] / 72;

        const approxEq = (a: number, b: number, epsilon = 0.01) => Math.abs(a - b) <= epsilon;

        expect(approxEq(width, paperFormats[format].width)).to.be.true();
        expect(approxEq(height, paperFormats[format].height)).to.be.true();
      }
    });

    it('with custom header and footer', async () => {
      await w.loadFile(path.join(fixturesPath, 'api', 'print-to-pdf-small.html'));

      const data = await w.webContents.printToPDF({
        displayHeaderFooter: true,
        headerTemplate: '<div>I\'m a PDF header</div>',
        footerTemplate: '<div>I\'m a PDF footer</div>'
      });

      const pdfInfo = await readPDF(data);

      expect(containsText(pdfInfo.textContent, /I'm a PDF header/)).to.be.true();
      expect(containsText(pdfInfo.textContent, /I'm a PDF footer/)).to.be.true();
    });

    it('in landscape mode', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'print-to-pdf-small.html'));

      const data = await w.webContents.printToPDF({ landscape: true });
      const pdfInfo = await readPDF(data);

      // page.view is [top, left, width, height].
      const width = pdfInfo.view[2];
      const height = pdfInfo.view[3];

      expect(width).to.be.greaterThan(height);
    });

    it('with custom page ranges', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'print-to-pdf-large.html'));

      const data = await w.webContents.printToPDF({
        pageRanges: '1-3',
        landscape: true
      });

      const pdfInfo = await readPDF(data);

      // Check that correct # of pages are rendered.
      expect(pdfInfo.numPages).to.equal(3);
    });

    it('does not tag PDFs by default', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'print-to-pdf-small.html'));

      const data = await w.webContents.printToPDF({});
      const pdfInfo = await readPDF(data);
      expect(pdfInfo.markInfo).to.be.null();
    });

    it('can generate tag data for PDFs', async () => {
      await w.loadFile(path.join(__dirname, 'fixtures', 'api', 'print-to-pdf-small.html'));

      const data = await w.webContents.printToPDF({ generateTaggedPDF: true });
      const pdfInfo = await readPDF(data);
      expect(pdfInfo.markInfo).to.deep.equal({
        Marked: true,
        UserProperties: false,
        Suspects: false
      });
    });

    it('from an existing pdf document', async () => {
      const pdfPath = path.join(fixturesPath, 'cat.pdf');
      const readyToPrint = once(w.webContents, '-pdf-ready-to-print');
      await w.loadFile(pdfPath);
      await readyToPrint;
      const data = await w.webContents.printToPDF({});
      const pdfInfo = await readPDF(data);
      expect(pdfInfo.numPages).to.equal(2);
      expect(containsText(pdfInfo.textContent, /Cat: The Ideal Pet/)).to.be.true();
    });

    it('from an existing pdf document in a WebView', async () => {
      const win = new BrowserWindow({
        show: false,
        webPreferences: {
          webviewTag: true
        }
      });

      await win.loadURL('about:blank');
      const webContentsCreated = once(app, 'web-contents-created') as Promise<[any, WebContents]>;

      const src = url.format({
        pathname: `${fixturesPath.replaceAll('\\', '/')}/cat.pdf`,
        protocol: 'file',
        slashes: true
      });
      await win.webContents.executeJavaScript(`
        new Promise((resolve, reject) => {
          const webview = new WebView()
          webview.setAttribute('src', '${src}')
          document.body.appendChild(webview)
          webview.addEventListener('did-finish-load', () => {
            resolve()
          })
        })
      `);

      const [, webContents] = await webContentsCreated;

      await once(webContents, '-pdf-ready-to-print');

      const data = await webContents.printToPDF({});
      const pdfInfo = await readPDF(data);
      expect(pdfInfo.numPages).to.equal(2);
      expect(containsText(pdfInfo.textContent, /Cat: The Ideal Pet/)).to.be.true();
    });
  });

  describe('PictureInPicture video', () => {
    afterEach(closeAllWindows);
    it('works as expected', async function () {
      const w = new BrowserWindow({ webPreferences: { sandbox: true } });

      // TODO(codebytere): figure out why this workaround is needed and remove.
      // It is not germane to the actual test.
      await w.loadFile(path.join(fixturesPath, 'blank.html'));

      await w.loadFile(path.join(fixturesPath, 'api', 'picture-in-picture.html'));

      await w.webContents.executeJavaScript('document.createElement(\'video\').canPlayType(\'video/webm; codecs="vp8.0"\')', true);

      const result = await w.webContents.executeJavaScript('runTest(true)', true);
      expect(result).to.be.true();
    });
  });

  describe('Shared Workers', () => {
    afterEach(closeAllWindows);

    it('can get multiple shared workers', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });

      const ready = once(ipcMain, 'ready');
      w.loadFile(path.join(fixturesPath, 'api', 'shared-worker', 'shared-worker.html'));
      await ready;

      const sharedWorkers = w.webContents.getAllSharedWorkers();

      expect(sharedWorkers).to.have.lengthOf(2);
      expect(sharedWorkers[0].url).to.contain('shared-worker');
      expect(sharedWorkers[1].url).to.contain('shared-worker');
    });

    it('can inspect a specific shared worker', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });

      const ready = once(ipcMain, 'ready');
      w.loadFile(path.join(fixturesPath, 'api', 'shared-worker', 'shared-worker.html'));
      await ready;

      const sharedWorkers = w.webContents.getAllSharedWorkers();

      const devtoolsOpened = once(w.webContents, 'devtools-opened');
      w.webContents.inspectSharedWorkerById(sharedWorkers[0].id);
      await devtoolsOpened;

      const devtoolsClosed = once(w.webContents, 'devtools-closed');
      w.webContents.closeDevTools();
      await devtoolsClosed;
    });
  });

  describe('login event', () => {
    afterEach(closeAllWindows);

    let server: http.Server;
    let serverUrl: string;
    let serverPort: number;
    let proxyServer: http.Server;
    let proxyServerPort: number;

    before(async () => {
      server = http.createServer((request, response) => {
        if (request.url === '/no-auth') {
          return response.end('ok');
        }
        if (request.headers.authorization) {
          response.writeHead(200, { 'Content-type': 'text/plain' });
          return response.end(request.headers.authorization);
        }
        response
          .writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' })
          .end('401');
      });
      ({ port: serverPort, url: serverUrl } = await listen(server));
    });

    before(async () => {
      proxyServer = http.createServer((request, response) => {
        if (request.headers['proxy-authorization']) {
          response.writeHead(200, { 'Content-type': 'text/plain' });
          return response.end(request.headers['proxy-authorization']);
        }
        response
          .writeHead(407, { 'Proxy-Authenticate': 'Basic realm="Foo"' })
          .end();
      });
      proxyServerPort = (await listen(proxyServer)).port;
    });

    afterEach(async () => {
      await session.defaultSession.clearAuthCache();
    });

    after(() => {
      server.close();
      server = null as unknown as http.Server;
      proxyServer.close();
      proxyServer = null as unknown as http.Server;
    });

    it('is emitted when navigating', async () => {
      const [user, pass] = ['user', 'pass'];
      const w = new BrowserWindow({ show: false });
      let eventRequest: any;
      let eventAuthInfo: any;
      w.webContents.on('login', (event, request, authInfo, cb) => {
        eventRequest = request;
        eventAuthInfo = authInfo;
        event.preventDefault();
        cb(user, pass);
      });
      await w.loadURL(serverUrl);
      const body = await w.webContents.executeJavaScript('document.documentElement.textContent');
      expect(body).to.equal(`Basic ${Buffer.from(`${user}:${pass}`).toString('base64')}`);
      expect(eventRequest.url).to.equal(serverUrl + '/');
      expect(eventAuthInfo.isProxy).to.be.false();
      expect(eventAuthInfo.scheme).to.equal('basic');
      expect(eventAuthInfo.host).to.equal('127.0.0.1');
      expect(eventAuthInfo.port).to.equal(serverPort);
      expect(eventAuthInfo.realm).to.equal('Foo');
    });

    it('is emitted when a proxy requests authorization', async () => {
      const customSession = session.fromPartition(`${Math.random()}`);
      await customSession.setProxy({ proxyRules: `127.0.0.1:${proxyServerPort}`, proxyBypassRules: '<-loopback>' });
      const [user, pass] = ['user', 'pass'];
      const w = new BrowserWindow({ show: false, webPreferences: { session: customSession } });
      let eventRequest: any;
      let eventAuthInfo: any;
      w.webContents.on('login', (event, request, authInfo, cb) => {
        eventRequest = request;
        eventAuthInfo = authInfo;
        event.preventDefault();
        cb(user, pass);
      });
      await w.loadURL(`${serverUrl}/no-auth`);
      const body = await w.webContents.executeJavaScript('document.documentElement.textContent');
      expect(body).to.equal(`Basic ${Buffer.from(`${user}:${pass}`).toString('base64')}`);
      expect(eventRequest.url).to.equal(`${serverUrl}/no-auth`);
      expect(eventAuthInfo.isProxy).to.be.true();
      expect(eventAuthInfo.scheme).to.equal('basic');
      expect(eventAuthInfo.host).to.equal('127.0.0.1');
      expect(eventAuthInfo.port).to.equal(proxyServerPort);
      expect(eventAuthInfo.realm).to.equal('Foo');
    });

    it('cancels authentication when callback is called with no arguments', async () => {
      const w = new BrowserWindow({ show: false });
      w.webContents.on('login', (event, request, authInfo, cb) => {
        event.preventDefault();
        cb();
      });
      await w.loadURL(serverUrl);
      const body = await w.webContents.executeJavaScript('document.documentElement.textContent');
      expect(body).to.equal('401');
    });
  });

  describe('page-title-updated event', () => {
    afterEach(closeAllWindows);
    it('is emitted with a full title for pages with no navigation', async () => {
      const bw = new BrowserWindow({ show: false });
      await bw.loadURL('about:blank');
      bw.webContents.executeJavaScript('child = window.open("", "", "show=no"); null');
      const [, child] = await once(app, 'web-contents-created') as [any, WebContents];
      bw.webContents.executeJavaScript('child.document.title = "new title"');
      const [, title] = await once(child, 'page-title-updated') as [any, string];
      expect(title).to.equal('new title');
    });
  });

  describe('context-menu event', () => {
    afterEach(closeAllWindows);
    it('emits when right-clicked in page', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadFile(path.join(fixturesPath, 'pages', 'base-page.html'));

      const promise = once(w.webContents, 'context-menu') as Promise<[any, Electron.ContextMenuParams]>;

      // Simulate right-click to create context-menu event.
      const opts = { x: 0, y: 0, button: 'right' as const };
      w.webContents.sendInputEvent({ ...opts, type: 'mouseDown' });
      w.webContents.sendInputEvent({ ...opts, type: 'mouseUp' });

      const [, params] = await promise;

      expect(params.pageURL).to.equal(w.webContents.getURL());
      expect(params.frame).to.be.an('object');
      expect(params.x).to.be.a('number');
      expect(params.y).to.be.a('number');
    });
  });

  describe('close() method', () => {
    afterEach(closeAllWindows);

    it('closes when close() is called', async () => {
      const w = (webContents as typeof ElectronInternal.WebContents).create();
      const destroyed = once(w, 'destroyed');
      w.close();
      await destroyed;
      expect(w.isDestroyed()).to.be.true();
    });

    it('closes when close() is called after loading a page', async () => {
      const w = (webContents as typeof ElectronInternal.WebContents).create();
      await w.loadURL('about:blank');
      const destroyed = once(w, 'destroyed');
      w.close();
      await destroyed;
      expect(w.isDestroyed()).to.be.true();
    });

    it('can be GCed before loading a page', async () => {
      const v8Util = process._linkedBinding('electron_common_v8_util');
      let registry: FinalizationRegistry<unknown> | null = null;
      const cleanedUp = new Promise<number>(resolve => {
        registry = new FinalizationRegistry(resolve as any);
      });
      (() => {
        const w = (webContents as typeof ElectronInternal.WebContents).create();
        registry!.register(w, 42);
      })();
      const i = setInterval(() => v8Util.requestGarbageCollectionForTesting(), 100);
      defer(() => clearInterval(i));
      expect(await cleanedUp).to.equal(42);
    });

    it('causes its parent browserwindow to be closed', async () => {
      const w = new BrowserWindow({ show: false });
      await w.loadURL('about:blank');
      const closed = once(w, 'closed');
      w.webContents.close();
      await closed;
      expect(w.isDestroyed()).to.be.true();
    });

    it('ignores beforeunload if waitForBeforeUnload not specified', async () => {
      const w = (webContents as typeof ElectronInternal.WebContents).create();
      await w.loadURL('about:blank');
      await w.executeJavaScript('window.onbeforeunload = () => "hello"; null');
      w.on('will-prevent-unload', () => { throw new Error('unexpected will-prevent-unload'); });
      const destroyed = once(w, 'destroyed');
      w.close();
      await destroyed;
      expect(w.isDestroyed()).to.be.true();
    });

    it('runs beforeunload if waitForBeforeUnload is specified', async () => {
      const w = (webContents as typeof ElectronInternal.WebContents).create();
      await w.loadURL('about:blank');
      await w.executeJavaScript('window.onbeforeunload = () => "hello"; null');
      const willPreventUnload = once(w, 'will-prevent-unload');
      w.close({ waitForBeforeUnload: true });
      await willPreventUnload;
      expect(w.isDestroyed()).to.be.false();
    });

    it('overriding beforeunload prevention results in webcontents close', async () => {
      const w = (webContents as typeof ElectronInternal.WebContents).create();
      await w.loadURL('about:blank');
      await w.executeJavaScript('window.onbeforeunload = () => "hello"; null');
      w.once('will-prevent-unload', e => e.preventDefault());
      const destroyed = once(w, 'destroyed');
      w.close({ waitForBeforeUnload: true });
      await destroyed;
      expect(w.isDestroyed()).to.be.true();
    });
  });

  describe('content-bounds-updated event', () => {
    afterEach(closeAllWindows);
    it('emits when moveTo is called', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      w.webContents.executeJavaScript('window.moveTo(50, 50)', true);
      const [, rect] = await once(w.webContents, 'content-bounds-updated') as [any, Electron.Rectangle];
      const { width, height } = w.getBounds();
      expect(rect).to.deep.equal({
        x: 50,
        y: 50,
        width,
        height
      });
      await new Promise(setImmediate);
      expect(w.getBounds().x).to.equal(50);
      expect(w.getBounds().y).to.equal(50);
    });

    it('emits when resizeTo is called', async () => {
      const w = new BrowserWindow({ show: false });
      w.loadURL('about:blank');
      w.webContents.executeJavaScript('window.resizeTo(100, 100)', true);
      const [, rect] = await once(w.webContents, 'content-bounds-updated') as [any, Electron.Rectangle];
      const { x, y } = w.getBounds();
      expect(rect).to.deep.equal({
        x,
        y,
        width: 100,
        height: 100
      });
      await new Promise(setImmediate);
      expect({
        width: w.getBounds().width,
        height: w.getBounds().height
      }).to.deep.equal(process.platform === 'win32'
        ? {
            // The width is reported as being larger on Windows? I'm not sure why
            // this is.
            width: 136,
            height: 100
          }
        : {
            width: 100,
            height: 100
          });
    });

    it('does not change window bounds if cancelled', async () => {
      const w = new BrowserWindow({ show: false });
      const { width, height } = w.getBounds();
      w.loadURL('about:blank');
      w.webContents.once('content-bounds-updated', e => e.preventDefault());
      await w.webContents.executeJavaScript('window.resizeTo(100, 100)', true);
      await new Promise(setImmediate);
      expect(w.getBounds().width).to.equal(width);
      expect(w.getBounds().height).to.equal(height);
    });
  });
});
