import { app, contentTracing, EnableHeapProfilingOptions, TraceConfig, TraceCategoriesAndOptions } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';
import * as fs from 'node:fs';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe, ifit, startRemoteControlApp } from './lib/spec-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');

// FIXME: The tests are skipped on linux arm/arm64
ifdescribe(!(['arm', 'arm64'].includes(process.arch)) || (process.platform !== 'linux'))('contentTracing', () => {
  const record = async (options: TraceConfig | TraceCategoriesAndOptions, outputFilePath: string | undefined, recordTimeInMilliseconds = 1e1) => {
    await app.whenReady();

    await contentTracing.startRecording(options);
    await setTimeout(recordTimeInMilliseconds);
    const resultFilePath = await contentTracing.stopRecording(outputFilePath);

    return resultFilePath;
  };

  const outputFilePath = path.join(app.getPath('temp'), 'trace.json');
  beforeEach(() => {
    if (fs.existsSync(outputFilePath)) {
      fs.unlinkSync(outputFilePath);
    }
  });

  describe('startRecording', function () {
    if (process.platform === 'win32' && process.arch === 'arm64') {
      // WOA needs more time
      this.timeout(10e3);
    } else {
      this.timeout(5e3);
    }

    const getFileSizeInKiloBytes = (filePath: string) => {
      const stats = fs.statSync(filePath);
      const fileSizeInBytes = stats.size;
      const fileSizeInKiloBytes = fileSizeInBytes / 1024;
      return fileSizeInKiloBytes;
    };

    it('accepts an empty config', async () => {
      const config = {};
      await record(config, outputFilePath);

      expect(fs.existsSync(outputFilePath)).to.be.true('output exists');

      const fileSizeInKiloBytes = getFileSizeInKiloBytes(outputFilePath);
      expect(fileSizeInKiloBytes).to.be.above(0,
        `the trace output file is empty, check "${outputFilePath}"`);
    });

    it('accepts a trace config', async () => {
      // (alexeykuzmin): All categories are excluded on purpose,
      // so only metadata gets into the output file.
      const config = {
        excluded_categories: ['*']
      };
      await record(config, outputFilePath);

      // If the `excluded_categories` param above is not respected, categories
      // like `node,node.environment` will be included in the output.
      const content = fs.readFileSync(outputFilePath).toString();
      expect(content.includes('"cat":"node,node.environment"')).to.be.false();
    });

    it('accepts "categoryFilter" and "traceOptions" as a config', async () => {
      // (alexeykuzmin): All categories are excluded on purpose,
      // so only metadata gets into the output file.
      const config = {
        categoryFilter: '__ThisIsANonexistentCategory__',
        traceOptions: ''
      };
      await record(config, outputFilePath);

      expect(fs.existsSync(outputFilePath)).to.be.true('output exists');

      // If the `categoryFilter` param above is not respected
      // the file size will be above 60KB.
      const fileSizeInKiloBytes = getFileSizeInKiloBytes(outputFilePath);
      const expectedMaximumFileSize = 60; // Depends on a platform.

      expect(fileSizeInKiloBytes).to.be.above(0,
        `the trace output file is empty, check "${outputFilePath}"`);
      expect(fileSizeInKiloBytes).to.be.below(expectedMaximumFileSize,
        `the trace output file is suspiciously large (${fileSizeInKiloBytes}KB),
        check "${outputFilePath}"`);
    });
  });

  ifdescribe(process.platform !== 'linux')('stopRecording', function () {
    if (process.platform === 'win32' && process.arch === 'arm64') {
      // WOA needs more time
      this.timeout(10e3);
    } else {
      this.timeout(5e3);
    }

    // FIXME(samuelmaddock): this test regularly flakes
    it.skip('does not crash on empty string', async () => {
      const options = {
        categoryFilter: '*',
        traceOptions: 'record-until-full,enable-sampling'
      };

      await contentTracing.startRecording(options);
      const path = await contentTracing.stopRecording('');
      expect(path).to.be.a('string').that.is.not.empty('result path');
      expect(fs.statSync(path).isFile()).to.be.true('output exists');
    });

    it('calls its callback with a result file path', async () => {
      const resultFilePath = await record(/* options */ {}, outputFilePath);
      expect(resultFilePath).to.be.a('string').and.be.equal(outputFilePath);
    });

    it('creates a temporary file when an empty string is passed', async function () {
      const resultFilePath = await record(/* options */ {}, /* outputFilePath */ '');
      expect(resultFilePath).to.be.a('string').that.is.not.empty('result path');
    });

    it('creates a temporary file when no path is passed', async function () {
      const resultFilePath = await record(/* options */ {}, /* outputFilePath */ undefined);
      expect(resultFilePath).to.be.a('string').that.is.not.empty('result path');
    });

    it('rejects if no trace is happening', async () => {
      await expect(contentTracing.stopRecording()).to.be.rejectedWith('Failed to stop tracing - no trace in progress');
    });
  });

  describe('getTraceBufferUsage', function () {
    this.timeout(10e3);

    it('does not crash and returns valid usage data', async () => {
      await app.whenReady();
      await contentTracing.startRecording({
        categoryFilter: '*',
        traceOptions: 'record-until-full'
      });

      // Yield to the event loop so the JS HandleScope from this tick is gone.
      // When the Mojo response arrives it fires OnTraceBufferUsageAvailable
      // as a plain Chromium task — if that callback lacks its own HandleScope
      // the process will crash with "Cannot create a handle without a HandleScope".
      const result = await contentTracing.getTraceBufferUsage();

      expect(result).to.have.property('percentage').that.is.a('number');
      expect(result).to.have.property('value').that.is.a('number');

      await contentTracing.stopRecording();
    });

    it('returns zero usage when no trace is active', async () => {
      await app.whenReady();
      const result = await contentTracing.getTraceBufferUsage();
      expect(result).to.have.property('percentage').that.is.a('number');
      expect(result.percentage).to.equal(0);
    });
  });

  describe('enableHeapProfiling', () => {
    const checkForHeapDumps = async (options?: EnableHeapProfilingOptions | false) => {
      const rc = await startRemoteControlApp();

      const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } = await rc.remotely(
        async (utilityProcessPath: string, options: EnableHeapProfilingOptions | false | undefined) => {
          const { contentTracing, BrowserWindow, utilityProcess } = require('electron');
          const { once } = require('node:events');
          const fs = require('node:fs');
          const process = require('node:process');
          const { setTimeout } = require('node:timers/promises');

          const isEventWithNonEmptyHeapDumpForProcess = (event: any, pid: number) =>
            event.cat === 'disabled-by-default-memory-infra' &&
            event.name === 'periodic_interval' &&
            event.pid === pid &&
            event.args.dumps.level_of_detail === 'detailed' &&
            event.args.dumps.process_mmaps?.vm_regions.length > 0 &&
            typeof event.args.dumps.allocators === 'object' &&
            typeof event.args.dumps.heaps_v2.allocators === 'object' &&
            Object.values(event.args.dumps.allocators).some((allocator: any) => allocator.attrs.size?.value !== '0') &&
            Object.values(event.args.dumps.heaps_v2.allocators).some(
              (allocator: any) =>
                allocator.counts.length > 0 && allocator.nodes.length > 0 && allocator.sizes.length > 0
            );

          const hasNonEmptyHeapDumpForProcess = (parsedTrace: any, pid: number) =>
            parsedTrace.traceEvents.some((event: any) => isEventWithNonEmptyHeapDumpForProcess(event, pid));

          if (options !== false) await contentTracing.enableHeapProfiling(options);

          const interval = 1000;
          await contentTracing.startRecording({
            included_categories: ['disabled-by-default-memory-infra'],
            excluded_categories: ['*'],
            memory_dump_config: {
              triggers: [{ mode: 'detailed', periodic_interval_ms: interval }]
            }
          });

          // Launch a renderer process
          const window = new BrowserWindow({ show: false });
          await window.webContents.loadURL('about:blank');

          // Launch a utility process
          const utility = utilityProcess.fork(utilityProcessPath);
          await once(utility, 'spawn');

          // Collect heap dumps
          await setTimeout(2 * interval);

          const path = await contentTracing.stopRecording();
          const data = fs.readFileSync(path, 'utf8');
          const parsed = JSON.parse(data);

          const hasBrowserProcessHeapDump = hasNonEmptyHeapDumpForProcess(parsed, process.pid);
          const hasRendererProcessHeapDump = hasNonEmptyHeapDumpForProcess(parsed, window.webContents.getOSProcessId());
          const hasUtilityProcessHeapDump = hasNonEmptyHeapDumpForProcess(parsed, utility.pid);

          global.setTimeout(() => require('electron').app.quit());

          return {
            hasBrowserProcessHeapDump,
            hasRendererProcessHeapDump,
            hasUtilityProcessHeapDump
          };
        },
        path.join(fixturesPath, 'api', 'content-tracing', 'utility.js'),
        options
      );

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);

      return {
        hasBrowserProcessHeapDump,
        hasRendererProcessHeapDump,
        hasUtilityProcessHeapDump
      };
    };

    it('does not include heap dumps when enableHeapProfiling is not called', async function () {
      const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } =
        await checkForHeapDumps(false);

      expect(hasBrowserProcessHeapDump).to.be.false();
      expect(hasRendererProcessHeapDump).to.be.false();
      expect(hasUtilityProcessHeapDump).to.be.false();
    });

    ifit(!process.env.IS_ASAN)(
      'includes heap dumps for browser process when called with { mode: "browser" }',
      async function () {
        const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } =
          await checkForHeapDumps({ mode: 'browser' });

        expect(hasBrowserProcessHeapDump).to.be.true();
        expect(hasRendererProcessHeapDump).to.be.false();
        expect(hasUtilityProcessHeapDump).to.be.false();
      }
    );

    ifit(!process.env.IS_ASAN)(
      'includes heap dumps for renderer processes when called with { mode: "all-renderers" }',
      async function () {
        const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } =
          await checkForHeapDumps({ mode: 'all-renderers' });

        expect(hasBrowserProcessHeapDump).to.be.false();
        expect(hasRendererProcessHeapDump).to.be.true();
        expect(hasUtilityProcessHeapDump).to.be.false();
      }
    );

    ifit(!process.env.IS_ASAN)(
      'includes heap dumps for utility processes when called with { mode: "all-utilities" }',
      async function () {
        const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } =
          await checkForHeapDumps({ mode: 'all-utilities' });

        expect(hasBrowserProcessHeapDump).to.be.false();
        expect(hasRendererProcessHeapDump).to.be.false();
        expect(hasUtilityProcessHeapDump).to.be.true();
      }
    );

    ifit(!process.env.IS_ASAN)(
      'includes heap dumps for browser, renderer, and utility processes when called with { mode: "all" }',
      async function () {
        const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } =
          await checkForHeapDumps({ mode: 'all' });

        expect(hasBrowserProcessHeapDump).to.be.true();
        expect(hasRendererProcessHeapDump).to.be.true();
        expect(hasUtilityProcessHeapDump).to.be.true();
      }
    );

    ifit(!process.env.IS_ASAN)(
      'includes heap dumps for browser, renderer, and utility processes when called without options',
      async function () {
        const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } =
          await checkForHeapDumps();

        expect(hasBrowserProcessHeapDump).to.be.true();
        expect(hasRendererProcessHeapDump).to.be.true();
        expect(hasUtilityProcessHeapDump).to.be.true();
      }
    );

    ifit(!process.env.IS_ASAN)('accepts valid options', async function () {
      const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } =
        await checkForHeapDumps({
          mode: 'all',
          stackMode: 'native-with-thread-names',
          samplingRate: 50000
        });

      expect(hasBrowserProcessHeapDump).to.be.true();
      expect(hasRendererProcessHeapDump).to.be.true();
      expect(hasUtilityProcessHeapDump).to.be.true();
    });

    ifit(!process.env.IS_ASAN)('does not crash when invalid options are passed', async function () {
      const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } =
        await checkForHeapDumps({
          // @ts-expect-error Invalid mode
          mode: 'invalid',
          // @ts-expect-error Invalid stack mode
          stackMode: 'invalid',
          samplingRate: -1000
        });

      expect(hasBrowserProcessHeapDump).to.be.true();
      expect(hasRendererProcessHeapDump).to.be.true();
      expect(hasUtilityProcessHeapDump).to.be.true();
    });

    ifit(!process.env.IS_ASAN)('does not crash when options of invalid types are passed', async function () {
      const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } =
        await checkForHeapDumps({
          // @ts-expect-error Invalid mode
          mode: { invalid: true },
          // @ts-expect-error Invalid stack mode
          stackMode: 999,
          // @ts-expect-error Invalid sampling rate
          samplingRate: 'invalid'
        });

      expect(hasBrowserProcessHeapDump).to.be.true();
      expect(hasRendererProcessHeapDump).to.be.true();
      expect(hasUtilityProcessHeapDump).to.be.true();
    });

    ifit(!!process.env.IS_ASAN)('does not include heap dumps in ASAN builds', async function () {
      const { hasBrowserProcessHeapDump, hasRendererProcessHeapDump, hasUtilityProcessHeapDump } =
        await checkForHeapDumps();

      expect(hasBrowserProcessHeapDump).to.be.false();
      expect(hasRendererProcessHeapDump).to.be.false();
      expect(hasUtilityProcessHeapDump).to.be.false();
    });

    ifit(!process.env.IS_ASAN)('rejects when called multiple times', async function () {
      const rc = await startRemoteControlApp();

      const [firstResult, secondResult, thirdResult] = await rc.remotely(async () => {
        const { contentTracing } = require('electron');

        // Call twice before enabling finishes.
        const firstPromise = contentTracing.enableHeapProfiling();
        const secondPromise = contentTracing.enableHeapProfiling();
        const [firstResult, secondResult] = await Promise.allSettled([firstPromise, secondPromise]);

        // Call again after enabling finishes.
        const thirdPromise = contentTracing.enableHeapProfiling();
        const [thirdResult] = await Promise.allSettled([thirdPromise]);

        global.setTimeout(() => require('electron').app.quit());

        return [firstResult, secondResult, thirdResult];
      });

      const [code] = await once(rc.process, 'exit');
      expect(code).to.equal(0);

      expect(firstResult.status).to.equal('fulfilled');
      expect(secondResult.status).to.equal('rejected');
      expect(secondResult.reason.message).to.equal('Heap profiling is already enabled');
      expect(thirdResult.status).to.equal('rejected');
      expect(thirdResult.reason.message).to.equal('Heap profiling is already enabled');
    });
  });

  describe('captured events', () => {
    it('include V8 samples from the main process', async function () {
      this.timeout(60000);
      await contentTracing.startRecording({
        categoryFilter: 'disabled-by-default-v8.cpu_profiler',
        traceOptions: 'record-until-full'
      });
      {
        const start = Date.now();
        let n = 0;
        const f = () => {};
        while (Date.now() - start < 200 && n < 500) {
          await setTimeout(0);
          f();
          n++;
        }
      }
      const path = await contentTracing.stopRecording();
      const data = fs.readFileSync(path, 'utf8');
      const parsed = JSON.parse(data);
      expect(parsed.traceEvents.some((x: any) => x.cat === 'disabled-by-default-v8.cpu_profiler' && x.name === 'ProfileChunk')).to.be.true();
    });
  });

  describe('trace metadata', () => {
    // These are necessary to be able to symbolicate heap dumps with third_party/catapult/tracing/bin/symbolize_trace.
    it('includes product version and OS arch metadata in JSON output', async () => {
      const config = {
        excluded_categories: ['*']
      };
      await record(config, outputFilePath);

      const content = fs.readFileSync(outputFilePath).toString();
      const parsed = JSON.parse(content);

      expect(parsed.metadata).to.be.an('object');
      expect(parsed.metadata['product-version']).to.be.a('string');
      expect(parsed.metadata['product-version'].startsWith(process.versions.chrome)).to.be.true();
      expect(parsed.metadata['os-arch']).to.be.a('string');
      expect(parsed.metadata['os-arch']).to.not.be.empty();
    });
  });
});
