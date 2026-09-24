import { app, contentTracing, type TraceConfig, type TraceCategoriesAndOptions } from 'electron/main';

import { expect } from 'chai';
import * as protobuf from 'protobufjs';

import { randomUUID } from 'node:crypto';
import * as fs from 'node:fs';
import * as path from 'node:path';
import { performance } from 'node:perf_hooks';
import { setTimeout } from 'node:timers/promises';
import * as vm from 'node:vm';

import { ifdescribe, waitUntil } from './lib/spec-helpers.ts';

// Test jobs do not include Chromium's source tree, so define the subset of the
// Perfetto schema these tests read. Field numbers match
// third_party/perfetto/protos/perfetto/trace/.
const perfettoTraceType = protobuf
  .parse(`
  syntax = "proto2";
  package perfetto.protos;
  message StackSample {}
  message EventCategory {
    optional uint64 iid = 1;
    optional string name = 2;
  }
  message EventName {
    optional uint64 iid = 1;
    optional string name = 2;
  }
  message InternedData {
    repeated EventCategory event_categories = 1;
    repeated EventName event_names = 2;
  }
  message LegacyEvent {
    optional int32 phase = 2;
  }
  message TrackEvent {
    repeated uint64 category_iids = 3;
    optional LegacyEvent legacy_event = 6;
    optional int32 type = 9;
    optional uint64 name_iid = 10;
    optional uint64 track_uuid = 11;
    repeated string categories = 22;
    optional string name = 23;
  }
  message ChromeMetadata {
    optional string name = 1;
    optional string string_value = 2;
  }
  message ChromeEventBundle {
    repeated ChromeMetadata metadata = 2;
  }
  message TracePacket {
    optional ChromeEventBundle chrome_events = 5;
    optional uint32 trusted_packet_sequence_id = 10;
    optional TrackEvent track_event = 11;
    optional InternedData interned_data = 12;
    optional uint32 sequence_flags = 13;
    optional StackSample stack_sample = 135;
  }
  message Trace {
    repeated TracePacket packet = 1;
  }
`)
  .root.lookupType('perfetto.protos.Trace');

interface InternedEntry {
  iid?: string;
  name?: string;
}

interface PerfettoPacket {
  chromeEvents?: { metadata?: Array<{ name?: string; stringValue?: string }> };
  trustedPacketSequenceId?: number;
  trackEvent?: {
    categoryIids?: string[];
    legacyEvent?: { phase?: number };
    type?: number;
    nameIid?: string;
    trackUuid?: string;
    categories?: string[];
    name?: string;
  };
  internedData?: { eventCategories?: InternedEntry[]; eventNames?: InternedEntry[] };
  sequenceFlags?: number;
  stackSample?: unknown;
}

const readPerfettoTrace = (filePath: string) =>
  perfettoTraceType.toObject(perfettoTraceType.decode(fs.readFileSync(filePath)), { longs: String }) as {
    packet?: PerfettoPacket[];
  };

// TrackEvent.Type values.
const TYPE_SLICE_BEGIN = 1;
const TYPE_SLICE_END = 2;
const TYPE_INSTANT = 3;
// TracePacket.SequenceFlags.SEQ_INCREMENTAL_STATE_CLEARED.
const SEQ_INCREMENTAL_STATE_CLEARED = 1;

interface TraceEvent {
  categories: string[];
  name?: string;
  type?: number;
  phase?: string;
  trackUuid?: string;
}

// Flattens the track events in a trace, resolving interned category and event
// names. Interning is scoped to a packet sequence and is reset whenever a
// packet clears the sequence's incremental state.
const getTraceEvents = (filePath: string): TraceEvent[] => {
  const interned = new Map<number, { categories: Map<string, string>; names: Map<string, string> }>();
  const events: TraceEvent[] = [];
  for (const packet of readPerfettoTrace(filePath).packet ?? []) {
    const sequenceId = packet.trustedPacketSequenceId ?? 0;
    if (!interned.has(sequenceId) || (packet.sequenceFlags ?? 0) & SEQ_INCREMENTAL_STATE_CLEARED) {
      interned.set(sequenceId, { categories: new Map(), names: new Map() });
    }
    const state = interned.get(sequenceId)!;
    for (const { iid, name } of packet.internedData?.eventCategories ?? []) {
      if (iid !== undefined && name !== undefined) state.categories.set(iid, name);
    }
    for (const { iid, name } of packet.internedData?.eventNames ?? []) {
      if (iid !== undefined && name !== undefined) state.names.set(iid, name);
    }

    const event = packet.trackEvent;
    if (!event) continue;
    // Category groups such as "node,node.environment" are split into their
    // individual categories.
    const categories = [
      ...(event.categories ?? []),
      ...(event.categoryIids ?? []).map((iid) => state.categories.get(iid) ?? '')
    ].flatMap((category) => category.split(','));
    const phase = event.legacyEvent?.phase;
    events.push({
      categories,
      name: event.name ?? (event.nameIid !== undefined ? state.names.get(event.nameIid) : undefined),
      type: event.type,
      phase: phase !== undefined ? String.fromCharCode(phase) : undefined,
      trackUuid: event.trackUuid
    });
  }
  return events;
};

const hasCategory = (event: TraceEvent, category: string) => event.categories.includes(category);

const getTraceMetadata = (filePath: string) =>
  new Map(
    (readPerfettoTrace(filePath).packet ?? [])
      .flatMap((packet) => packet.chromeEvents?.metadata ?? [])
      .map(({ name, stringValue }) => [name, stringValue] as const)
  );

// FIXME: The tests are skipped on linux arm64
ifdescribe(process.arch !== 'arm64' || process.platform !== 'linux')('contentTracing', () => {
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

  // Every test attempt (including mocha retries) writes to its own file. A
  // previous attempt that timed out can still have a trace endpoint writing to
  // its output path in the background, and two endpoints finalizing the same
  // path race on the final rename.
  let outputFilePath: string;
  beforeEach(() => {
    outputFilePath = path.join(app.getPath('temp'), `electron-content-tracing-${randomUUID()}.pftrace`);
  });
  afterEach(() => {
    fs.rmSync(outputFilePath, { force: true });
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
      const events = getTraceEvents(outputFilePath);
      expect(events.some((event) => hasCategory(event, 'node.environment'))).to.be.false();
    });

    it('records the Perfetto protobuf format', async () => {
      await record({}, outputFilePath);

      expect(fs.readFileSync(outputFilePath)[0]).to.not.equal('{'.charCodeAt(0), 'output looks like JSON');
      expect(readPerfettoTrace(outputFilePath).packet).to.be.an('array').that.is.not.empty('trace has no packets');
    });

    it('rejects invalid heap profiler options', () => {
      expect(() =>
        contentTracing.startRecording({
          heap_profiler_options: {
            sampling_interval_bytes: 0
          }
        })
      ).to.throw();
      expect(() =>
        contentTracing.startRecording({
          heap_profiler_options: {
            sampling_interval_bytes: -1
          }
        })
      ).to.throw();
      expect(() =>
        contentTracing.startRecording({
          heap_profiler_options: {
            dump_interval_ms: 1.5
          }
        })
      ).to.throw();
      expect(() => contentTracing.startRecording({ heap_profiler_options: 'invalid' } as any)).to.throw();
      expect(() =>
        contentTracing.startRecording({
          heap_profiler_options: {
            dump_interval_ms: 2 ** 32
          }
        })
      ).to.throw();
    });

    it('rejects a second stop while heap tracing is stopping', async () => {
      await contentTracing.startRecording({ heap_profiler_options: {} });

      const firstStop = contentTracing.stopRecording(outputFilePath);
      await expect(contentTracing.stopRecording(`${outputFilePath}.second`)).to.eventually.be.rejectedWith(
        'Failed to stop tracing'
      );
      await firstStop;
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

      const fileSizeInKiloBytes = getFileSizeInKiloBytes(outputFilePath);
      expect(fileSizeInKiloBytes).to.be.above(0, `the trace output file is empty, check "${outputFilePath}"`);

      // If the `categoryFilter` param above is not respected the trace will
      // contain events from other categories. Metadata events use the
      // always-enabled `__metadata` category.
      const expectedCategories = new Set(['__ThisIsANonexistentCategory__', '__metadata']);
      const unexpectedCategories = new Set(
        getTraceEvents(outputFilePath)
          .flatMap((event) => event.categories)
          .filter((category) => !expectedCategories.has(category))
      );
      expect([...unexpectedCategories]).to.be.empty(`unexpected trace event categories, check "${outputFilePath}"`);
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

    it('settles concurrent requests during heap profiling', async () => {
      await app.whenReady();
      await contentTracing.startRecording({ heap_profiler_options: {} });

      const results = await Promise.all([contentTracing.getTraceBufferUsage(), contentTracing.getTraceBufferUsage()]);
      for (const result of results) {
        expect(result).to.have.property('percentage').that.is.a('number');
        expect(result).to.have.property('value').that.is.a('number');
      }

      await contentTracing.stopRecording();
    });
  });

  describe('captured events', () => {
    it('include native heap profiler stack samples', async function () {
      this.timeout(60000);
      await app.whenReady();
      await contentTracing.startRecording({
        heap_profiler_options: {
          dump_interval_ms: 10,
          sampling_interval_bytes: 1024
        }
      });

      const allocations: Buffer[] = [];
      for (let index = 0; index < 1000; index++) {
        allocations.push(Buffer.alloc(4096));
      }
      await waitUntil(async () => (await contentTracing.getTraceBufferUsage()).percentage > 0);

      await contentTracing.stopRecording(outputFilePath);
      const trace = readPerfettoTrace(outputFilePath);
      expect(trace.packet?.some((packet) => packet.stackSample !== undefined)).to.be.true();
    });

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
      const events = getTraceEvents(path);
      expect(
        events.some(
          (event) => hasCategory(event, 'disabled-by-default-v8.cpu_profiler') && event.name === 'ProfileChunk'
        )
      ).to.be.true();
    });
  });

  describe('node trace categories', () => {
    it('captures performance.mark() as instant trace events', async function () {
      await contentTracing.startRecording({
        included_categories: ['node.perf.usertiming']
      });

      performance.mark('test-trace-mark');

      const resultPath = await contentTracing.stopRecording();
      const markEvents = getTraceEvents(resultPath).filter(
        (event) => hasCategory(event, 'node.perf.usertiming') && event.name === 'test-trace-mark'
      );
      expect(markEvents).to.have.lengthOf.at.least(1, 'should have node.perf.usertiming events for performance.mark()');
      // Instants are typed events, or legacy events with the 'I' phase.
      expect(markEvents[0].type === TYPE_INSTANT || markEvents[0].phase === 'I').to.be.true(
        'performance.mark() should emit instant events'
      );
    });

    it('captures performance.measure() as nestable async begin/end trace events', async function () {
      await contentTracing.startRecording({
        included_categories: ['node.perf.usertiming']
      });

      performance.mark('trace-measure-start');
      await setTimeout(100);
      performance.mark('trace-measure-end');
      performance.measure('test-trace-measure', 'trace-measure-start', 'trace-measure-end');

      const resultPath = await contentTracing.stopRecording();
      const events = getTraceEvents(resultPath);

      // Nestable async slices are typed begin/end events on an async track, or
      // legacy events with the 'b'/'e' phases. Typed end events carry no name,
      // so they are matched to the begin event by track.
      const begin = events.find(
        (event) =>
          hasCategory(event, 'node.perf.usertiming') &&
          event.name === 'test-trace-measure' &&
          (event.type === TYPE_SLICE_BEGIN || event.phase === 'b')
      );
      expect(begin).to.not.be.undefined('should have a nestable async begin event');
      const hasEnd = events.some(
        (event) =>
          (event.type === TYPE_SLICE_END && event.trackUuid === begin!.trackUuid) ||
          (event.phase === 'e' && event.name === 'test-trace-measure')
      );
      expect(hasEnd).to.be.true('should have a nestable async end event');
    });

    it('captures node.fs.sync trace events for file operations', async function () {
      await contentTracing.startRecording({
        included_categories: ['node.fs.sync']
      });

      fs.readFileSync(import.meta.filename, 'utf8');

      const resultPath = await contentTracing.stopRecording();
      const fsEvents = getTraceEvents(resultPath).filter((event) => hasCategory(event, 'node.fs.sync'));
      expect(fsEvents).to.have.lengthOf.at.least(1, 'should have node.fs.sync trace events');
    });

    it('captures multiple node categories simultaneously', async function () {
      await contentTracing.startRecording({
        included_categories: ['node.async_hooks', 'node.vm.script']
      });

      vm.runInNewContext('1 + 1');
      await fs.promises.readFile(import.meta.filename, 'utf8');

      const resultPath = await contentTracing.stopRecording();
      const events = getTraceEvents(resultPath);

      const asyncHooksEvents = events.filter((event) => hasCategory(event, 'node.async_hooks'));
      const vmEvents = events.filter((event) => hasCategory(event, 'node.vm.script'));
      expect(asyncHooksEvents).to.have.lengthOf.at.least(1, 'should have node.async_hooks events');
      expect(vmEvents).to.have.lengthOf.at.least(1, 'should have node.vm.script events');
    });

    it('captures events using wildcard category pattern node.fs.*', async function () {
      await contentTracing.startRecording({
        included_categories: ['node.fs.*']
      });

      fs.readFileSync(import.meta.filename, 'utf8');
      await fs.promises.readFile(import.meta.filename, 'utf8');

      const resultPath = await contentTracing.stopRecording();
      const events = getTraceEvents(resultPath);

      const syncEvents = events.filter((event) => hasCategory(event, 'node.fs.sync'));
      const asyncEvents = events.filter((event) => hasCategory(event, 'node.fs.async'));
      expect(syncEvents).to.have.lengthOf.at.least(1, 'should have node.fs.sync events from wildcard pattern');
      expect(asyncEvents).to.have.lengthOf.at.least(1, 'should have node.fs.async events from wildcard pattern');
    });
  });

  describe('trace metadata', () => {
    // These are necessary to be able to symbolicate heap dumps with third_party/catapult/tracing/bin/symbolize_trace.
    it('includes product version and OS arch metadata', async () => {
      const config = {
        excluded_categories: ['*']
      };
      await record(config, outputFilePath);

      const metadata = getTraceMetadata(outputFilePath);
      const productVersion = metadata.get('product-version');
      expect(productVersion).to.be.a('string');
      expect(productVersion!.startsWith(process.versions.chrome)).to.be.true();
      expect(metadata.get('os-arch')).to.be.a('string').that.is.not.empty();
    });
  });
});
