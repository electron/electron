const { app, BrowserWindow, sharedTexture } = require('electron');
const assert = require('node:assert/strict');
const { fstatSync, readdirSync } = require('node:fs');
const { join } = require('node:path');
const native = process.env.ELECTRON_NATIVE_FRAME_HARDWARE_TESTS === '1';

const layout = process.env.ELECTRON_NATIVE_FRAME_LAYOUT ?? 'linear';
if (native) {
  app.commandLine.appendSwitch(
    'enable-features',
    layout === 'tiled' ? 'CanvasNativeVideoFrameExport:linear/false' : 'CanvasNativeVideoFrameExport:linear/true'
  );
}
const timeout = setTimeout(() => app.exit(1), 45000);
let source,
  consumer,
  paintEvents = 0;
const results = [];

app.whenReady().then(async () => {
  const create = () => {
    const window = new BrowserWindow({
      show: false,
      webPreferences: {
        preload: join(__dirname, 'preload.cjs'),
        sandbox: true,
        contextIsolation: true,
        backgroundThrottling: false,
        offscreen: { useSharedTexture: true }
      }
    });
    window.webContents.stopPainting();
    window.webContents.on('paint', (event) => {
      paintEvents++;
      event.texture?.release();
    });
    window.webContents.on('console-message', (event) => console.error(event.message));
    return window;
  };
  try {
    source = create();
    consumer = create();
    await Promise.all([source, consumer].map((window) => window.loadURL('data:text/html,<body></body>')));
    const js = (code) => source.webContents.executeJavaScript(code);
    await js(`window.canvas = document.createElement('canvas');
      canvas.width=512; canvas.height=256;document.body.append(canvas);
      window.ctx=canvas.getContext('2d',{alpha:false});
      ctx.fillStyle='red'; ctx.fillRect(0,0,256,256);
      ctx.fillStyle='blue'; ctx.fillRect(256,0,256,256);`);
    await assert.rejects(js('nativeHandleTest.export({})'), /VideoFrame/);
    await assert.rejects(
      js(`(()=>{const f=new VideoFrame(canvas,{timestamp:0});f.close();return nativeHandleTest.export(f)})()`),
      /closed/
    );
    await assert.rejects(
      js(`(()=>{const f=new VideoFrame(new Uint8Array(16),{format:'RGBA',codedWidth:2,codedHeight:2,timestamp:0});
      const p=nativeHandleTest.export(f);f.close();return p})()`),
      /GPU/
    );
    results.push('invalid, closed and CPU frames rejected');
    const exportFrame = 'nativeHandleTest.export(new VideoFrame(canvas,{timestamp:1234}),true)';
    if (!native) {
      assert.throws(() => sharedTexture.resolveVideoFrameHandle(source.webContents.mainFrame, 'missing'), /Unknown/);
      results.push('unknown ID rejected');
    } else {
      const pair = await js(`Promise.all([
        ${exportFrame},
        ${exportFrame}.then(()=>'', error=>error.message)
      ])`);
      assert.match(pair[1], /already pending/);
      await js(`nativeHandleTest.release(${JSON.stringify(pair[0])})`);
      results.push('concurrent export rejects without invalidating its pending peer');
      const pending = [];
      for (let index = 0; index < 4; index++) pending.push(await js(exportFrame));
      await assert.rejects(js(exportFrame), /too many pending/);
      for (const token of pending) await js(`nativeHandleTest.release(${JSON.stringify(token)})`);
      const id = await js(exportFrame);
      for (const token of pending) {
        assert.throws(() => sharedTexture.resolveVideoFrameHandle(source.webContents.mainFrame, token), /Unknown/);
      }
      results.push('four-export bound enforced; cancellation releases pending handles');
      assert.match(id, /^[a-f\d-]{36}$/);
      assert.throws(() => sharedTexture.resolveVideoFrameHandle(consumer.webContents.mainFrame, id), /Unknown/);
      const lease = sharedTexture.resolveVideoFrameHandle(source.webContents.mainFrame, id);
      assert.throws(() => sharedTexture.resolveVideoFrameHandle(source.webContents.mainFrame, id), /Unknown/);
      results.push('native plane count: ' + lease.textureInfo.handle.nativePixmap.planes.length);
      const fd = lease.textureInfo.handle.nativePixmap.planes[0].fd;
      fstatSync(fd);
      results.push('native handle resolves once, only for its owning document');
      await js("ctx.fillStyle='green';ctx.fillRect(0,0,512,256)");
      // Verify that closing the JS frame and reusing its canvas did not recycle
      // the exported allocation. Import/CPU readback belongs only to this test.
      const imported = sharedTexture.importSharedTexture({ textureInfo: lease.textureInfo });
      await sharedTexture.sendSharedTexture({ importedSharedTexture: imported, frame: consumer.webContents.mainFrame });
      const pixels = await consumer.webContents.executeJavaScript('nativeHandleTest.pixels()');
      assert.deepEqual(pixels, [
        [255, 0, 0, 255],
        [0, 0, 255, 255]
      ]);
      imported.release();
      results.push('source retained after early close and canvas redraw; exact pixels preserved');
      const unresolved = await js(exportFrame);
      await source.loadURL('data:text/html,<body>new document</body>');
      assert.throws(() => sharedTexture.resolveVideoFrameHandle(source.webContents.mainFrame, unresolved), /Unknown/);
      results.push('navigation invalidates unresolved IDs');
      fstatSync(fd);
      await lease.release();
      await lease.release();
      assert.throws(() => fstatSync(fd), { code: 'EBADF' });
      results.push('resolved lease survives navigation and explicitly closes its descriptor');
      await js(`window.canvas=document.createElement('canvas');canvas.width=512;canvas.height=256;
        document.body.append(canvas);window.ctx=canvas.getContext('2d',{alpha:false});ctx.fillRect(0,0,512,256)`);
      const warm = await js(exportFrame);
      await sharedTexture.resolveVideoFrameHandle(source.webContents.mainFrame, warm).release();
      const descriptors = readdirSync('/proc/self/fd').length;
      const rendererDescriptors = () => readdirSync(`/proc/${source.webContents.getOSProcessId()}/fd`).length;
      const beforeRenderer = rendererDescriptors();
      for (let index = 0; index < 20; index++) {
        const next = await js(exportFrame);
        const item = sharedTexture.resolveVideoFrameHandle(source.webContents.mainFrame, next);
        await item.release();
      }
      assert.ok(readdirSync('/proc/self/fd').length <= descriptors + 2, 'native descriptor leak');
      assert.ok(rendererDescriptors() <= beforeRenderer + 2, 'renderer descriptor leak');
      results.push('20 repeated export/release cycles without main or renderer descriptor growth');
      const expired = await js(exportFrame);
      await new Promise((resolve) => setTimeout(resolve, 16000));
      assert.throws(() => sharedTexture.resolveVideoFrameHandle(source.webContents.mainFrame, expired), /Unknown/);
      results.push('unresolved export expires after its deadline');
    }
    assert.equal(paintEvents, 0);
    const report = {
      native,
      layout,
      results,
      paintEvents,
      versions: process.versions,
      gpuFeatures: app.getGPUFeatureStatus()
    };
    console.log(JSON.stringify(report));
    clearTimeout(timeout);
    source.destroy();
    consumer.destroy();
    app.exit(0);
  } catch (error) {
    console.error(error);
    app.exit(1);
  }
});
