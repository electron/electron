import { app, contentTracing, TraceConfig, TraceCategoriesAndOptions } from 'electron/main';

import { expect } from 'chai';

import * as fs from 'node:fs';
import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifdescribe } from './lib/spec-helpers';

// FIXME: The tests are skipped on linux arm/arm64
ifdescribe(!['arm', 'arm64'].includes(process.arch) || process.platform !== 'linux')('contentTracing', () => {
  const record = async (
    options: TraceConfig | TraceCategoriesAndOptions,
    outputFilePath: string | undefined,
    recordTimeInMilliseconds = 1e1
  ) => {
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
      expect(fileSizeInKiloBytes).to.be.above(0, `the trace output file is empty, check "${outputFilePath}"`);
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

      expect(fileSizeInKiloBytes).to.be.above(0, `the trace output file is empty, check "${outputFilePath}"`);
      expect(fileSizeInKiloBytes).to.be.below(
        expectedMaximumFileSize,
        `the trace output file is suspiciously large (${fileSizeInKiloBytes}KB),
        check "${outputFilePath}"`
      );
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
      expect(
        parsed.traceEvents.some(
          (x: any) => x.cat === 'disabled-by-default-v8.cpu_profiler' && x.name === 'ProfileChunk'
        )
      ).to.be.true();
    });
  });

  describe('node trace categories', () => {
    it('captures performance.mark() as instant trace events', async function () {
      const { performance } = require('node:perf_hooks');

      await contentTracing.startRecording({
        included_categories: ['node.perf.usertiming']
      });

      performance.mark('test-trace-mark');

      const resultPath = await contentTracing.stopRecording();
      const data = fs.readFileSync(resultPath, 'utf8');
      const parsed = JSON.parse(data);

      const markEvents = parsed.traceEvents.filter(
        (x: any) => x.cat === 'node.perf.usertiming' && x.name === 'test-trace-mark'
      );
      expect(markEvents).to.have.lengthOf.at.least(1, 'should have node.perf.usertiming events for performance.mark()');
      expect(markEvents[0].ph).to.equal('I', 'performance.mark() should emit instant (I) phase events');
    });

    it('captures performance.measure() as nestable async begin/end trace events', async function () {
      const { performance } = require('node:perf_hooks');

      await contentTracing.startRecording({
        included_categories: ['node.perf.usertiming']
      });

      performance.mark('trace-measure-start');
      await setTimeout(100);
      performance.mark('trace-measure-end');
      performance.measure('test-trace-measure', 'trace-measure-start', 'trace-measure-end');

      const resultPath = await contentTracing.stopRecording();
      const data = fs.readFileSync(resultPath, 'utf8');
      const parsed = JSON.parse(data);

      const measureEvents = parsed.traceEvents.filter(
        (x: any) => x.cat === 'node.perf.usertiming' && x.name === 'test-trace-measure'
      );
      expect(measureEvents.some((x: any) => x.ph === 'b')).to.be.true('should have nestable async begin (b) event');
      expect(measureEvents.some((x: any) => x.ph === 'e')).to.be.true('should have nestable async end (e) event');
    });

    it('captures node.fs.sync trace events for file operations', async function () {
      await contentTracing.startRecording({
        included_categories: ['node.fs.sync']
      });

      fs.readFileSync(__filename, 'utf8');

      const resultPath = await contentTracing.stopRecording();
      const data = fs.readFileSync(resultPath, 'utf8');
      const parsed = JSON.parse(data);

      const fsEvents = parsed.traceEvents.filter(
        (x: any) => typeof x.cat === 'string' && x.cat.includes('node.fs.sync')
      );
      expect(fsEvents).to.have.lengthOf.at.least(1, 'should have node.fs.sync trace events');
    });

    it('captures multiple node categories simultaneously', async function () {
      const vm = require('node:vm');

      await contentTracing.startRecording({
        included_categories: ['node.async_hooks', 'node.vm.script']
      });

      vm.runInNewContext('1 + 1');
      await fs.promises.readFile(__filename, 'utf8');

      const resultPath = await contentTracing.stopRecording();
      const data = fs.readFileSync(resultPath, 'utf8');
      const parsed = JSON.parse(data);

      const asyncHooksEvents = parsed.traceEvents.filter(
        (x: any) => typeof x.cat === 'string' && x.cat.includes('node.async_hooks')
      );
      const vmEvents = parsed.traceEvents.filter(
        (x: any) => typeof x.cat === 'string' && x.cat.includes('node.vm.script')
      );
      expect(asyncHooksEvents).to.have.lengthOf.at.least(1, 'should have node.async_hooks events');
      expect(vmEvents).to.have.lengthOf.at.least(1, 'should have node.vm.script events');
    });

    it('captures events using wildcard category pattern node.fs.*', async function () {
      await contentTracing.startRecording({
        included_categories: ['node.fs.*']
      });

      fs.readFileSync(__filename, 'utf8');
      await fs.promises.readFile(__filename, 'utf8');

      const resultPath = await contentTracing.stopRecording();
      const data = fs.readFileSync(resultPath, 'utf8');
      const parsed = JSON.parse(data);

      const syncEvents = parsed.traceEvents.filter(
        (x: any) => typeof x.cat === 'string' && x.cat.includes('node.fs.sync')
      );
      const asyncEvents = parsed.traceEvents.filter(
        (x: any) => typeof x.cat === 'string' && x.cat.includes('node.fs.async')
      );
      expect(syncEvents).to.have.lengthOf.at.least(1, 'should have node.fs.sync events from wildcard pattern');
      expect(asyncEvents).to.have.lengthOf.at.least(1, 'should have node.fs.async events from wildcard pattern');
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
