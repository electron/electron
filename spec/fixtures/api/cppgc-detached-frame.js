const assert = require('node:assert/strict');
const { mkdtemp, readFile, rm, unlink } = require('node:fs/promises');
const { tmpdir } = require('node:os');
const { join } = require('node:path');
const { pathToFileURL } = require('node:url');

const { BrowserWindow } = require('electron');

module.exports = async (page, child, snapshotHelper, mode) => {
  const { countHeapSnapshotNodes } = require(snapshotHelper);
  const snapshotDir = await mkdtemp(join(tmpdir(), 'electron-detached-frame-'));
  const snapshotPath = join(snapshotDir, 'renderer.heapsnapshot');
  const window = new BrowserWindow({
    show: false,
    webPreferences: {
      nodeIntegration: true,
      nodeIntegrationInSubFrames: true,
      contextIsolation: false,
      sandbox: false
    }
  });
  const evaluate = (source) => window.webContents.executeJavaScript(source);
  const collect = () =>
    evaluate(`
    (async () => {
      const v8Util = process._linkedBinding('electron_common_v8_util');
      for (let i = 0; i < 3; ++i) {
        await new Promise(resolve => setTimeout(resolve, 0));
        v8Util.requestGarbageCollectionForTesting();
      }
      return process._linkedBinding('electron_common_testing')
        .getLiveCallbackHolderProbeCountForTesting();
    })()
  `);
  const snapshot = async () => {
    await collect();
    await window.webContents.takeHeapSnapshot(snapshotPath);
    try {
      // Snapshot generation collects garbage and completes cppgc sweeping.
      // Read the destructor count afterwards to match the snapshot's state.
      const probes = await evaluate(`
        process._linkedBinding('electron_common_testing')
          .getLiveCallbackHolderProbeCountForTesting()
      `);
      const counts = {
        probes,
        ...countHeapSnapshotNodes(await readFile(snapshotPath), {
          markers: { name: 'DetachedFrameCacheSentinel', type: 'object' },
          objectTemplates: { name: 'system / ObjectTemplateInfo' },
          functionTemplates: { name: 'system / FunctionTemplateInfo' },
          holders: { name: 'Electron / CallbackHolder' }
        })
      };
      for (const key of ['objectTemplates', 'functionTemplates', 'holders']) {
        assert.ok(counts[key] > 0, `snapshot must expose ${key} for the growth check`);
      }
      return counts;
    } finally {
      await unlink(snapshotPath);
    }
  };
  const createImages = (count, retain = false) =>
    evaluate(`
    (() => {
      const state = globalThis.detachedFrameState;
      for (let i = 0; i < ${count}; ++i) {
        const image = state.nativeImage.createEmpty();
        const size = image.getSize();
        if (!image.isEmpty() || size.width !== 0 || size.height !== 0)
          throw new Error('NativeImage methods returned unexpected results');
        if (${retain})
          state.images.push(image);
      }
      return ${count};
    })()
  `);
  const detach = async () => {
    const hasContextData = await evaluate(`
      (async () => {
        document.querySelector('iframe').remove();
        await new Promise(resolve => setTimeout(resolve, 200));
        return globalThis.detachedFrameState.getGinData().hasContextData;
      })()
    `);
    assert.equal(hasContextData, false, 'the child gin cache must be detached before exercising the API');
  };
  const verifyRelease = async (baselineProbes) => {
    await evaluate(`
      globalThis.detachedFrameState.nativeImage = null;
      globalThis.detachedFrameState.getGinData = null;
    `);
    const imagesOnly = await snapshot();
    assert.equal(imagesOnly.markers, 1, 'retained image methods must keep their creation realm reachable');
    assert.equal(imagesOnly.probes, baselineProbes + 1);
    await evaluate(`
      for (const image of globalThis.detachedFrameState.images) {
        if (!image.isEmpty())
          throw new Error('retained image is no longer callable');
      }
      delete globalThis.detachedFrameState;
    `);
    const released = await snapshot();
    assert.equal(released.markers, 0, 'dropping the API and images must release the child realm');
    assert.equal(released.probes, baselineProbes, 'the child native callback resource must be destroyed');
    return { imagesOnly, released };
  };
  let onCrash;
  const crashed = new Promise((resolve, reject) => {
    onCrash = (event, details) => reject(new Error(`detached frame renderer crashed: ${details.reason}`));
  });
  window.webContents.on('render-process-gone', onCrash);

  const run = async () => {
    await window.loadFile(page);
    // Warm the parent's testing bindings before measuring the baseline.
    const baselineProbes = await collect();
    await evaluate(`
      (async () => {
        const frame = document.createElement('iframe');
        const loaded = new Promise((resolve, reject) => {
          frame.onload = resolve;
          frame.onerror = () => reject(new Error('failed to load child frame'));
        });
        frame.src = ${JSON.stringify(pathToFileURL(child).href)};
        document.body.appendChild(frame);
        await loaded;
        const testing = frame.contentWindow.process._linkedBinding('electron_common_testing');
        if (!testing.getGinDataForTesting().hasContextData)
          throw new Error('child frame did not initialize its gin cache');
        if (frame.contentWindow.detachedFrameCacheSentinel.probe.run() !== 42)
          throw new Error('child callback probe did not initialize');
        globalThis.detachedFrameState = {
          nativeImage: frame.contentWindow.require('electron').nativeImage,
          getGinData: testing.getGinDataForTesting,
          images: []
        };
      })()
    `);

    if (mode === 'release') {
      // Create wrappers before detach so collection is tested independently
      // of the regression that creates a new wrapper after detach.
      await createImages(1, true);
      await detach();
      return verifyRelease(baselineProbes);
    }

    await detach();
    if (mode === 'call') {
      await createImages(1, true);
      await collect();
      const empty = await evaluate('globalThis.detachedFrameState.images[0].isEmpty()');
      assert.equal(empty, true);
      return empty;
    }

    assert.equal(mode, 'growth');
    await createImages(8);
    const warm = await snapshot();
    assert.equal(warm.markers, 1);
    assert.equal(warm.probes, baselineProbes + 1);
    const batches = [32, 64];
    const samples = [];
    for (const count of batches) {
      await createImages(count);
      const after = await snapshot();
      samples.push(after);
      assert.equal(after.markers, 1, 'growth must be measured while the detached realm is still retained');
      assert.equal(after.probes, baselineProbes + 1);
      for (const key of ['objectTemplates', 'functionTemplates', 'holders']) {
        // Allow background variation, but reject even one retained template
        // or callback holder per four creations after the warmup.
        assert.ok(
          after[key] - warm[key] < count / 4,
          `${key} grew by ${after[key] - warm[key]} after ${count} detached-frame creations`
        );
      }
    }
    await createImages(1, true);
    const release = await verifyRelease(baselineProbes);
    return { warm, batches, samples, release };
  };

  try {
    return await Promise.race([run(), crashed]);
  } finally {
    window.webContents.removeListener('render-process-gone', onCrash);
    window.destroy();
    await rm(snapshotDir, { recursive: true, force: true });
  }
};
