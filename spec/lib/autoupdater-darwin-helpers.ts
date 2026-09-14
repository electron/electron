import psList from 'ps-list';

import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as http from 'node:http';
import { AddressInfo } from 'node:net';
import * as os from 'node:os';
import * as path from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';

import {
  copyMacOSFixtureApp,
  getCodesignIdentity,
  shouldRunCodesignTests,
  signApp,
  spawn,
  stripFrameworkSymbols
} from './codesign-helpers';
import { withTempDirectory } from './fs-helpers';
import { createRoutedServer, RoutedRequest, RoutedServer } from './http-server-helpers';

// The Squirrel.Mac updater specs are split across files so that shards can
// run them in parallel on separate runners; this is the harness they share.
// We can only test the auto updater on darwin non-component builds.
export const shouldRunUpdaterSpecs = shouldRunCodesignTests && !process.env.IS_UBSAN;

// How many fixture apps may be updating at once within one spec file; each
// is about a core of codesign/ditto/ShipIt work. Set to 1 to run every test's
// work inline, in mocha's order.
const CONCURRENCY = (() => {
  const fromEnv = parseInt(process.env.ELECTRON_SPEC_UPDATER_CONCURRENCY || '', 10);
  if (fromEnv > 0) return fromEnv;
  // The Intel CI runners have three cores and each run saturates them, so
  // running two at once there gained under a minute per file and made every
  // run slower and less predictable. The parallelism comes from the files
  // landing on different shards instead.
  if (process.arch === 'x64') return 1;
  return os.availableParallelism() >= 8 ? 4 : 2;
})();

// A run may hold its slot for this multiple of its test's timeout, counted from
// when it gets the slot. Time spent queued for a slot does not count.
// ELECTRON_SPEC_UPDATER_RUN_BUDGET_MS overrides it, to exercise the abort path.
const RUN_BUDGET_MULTIPLIER = 2;
const RUN_BUDGET_OVERRIDE_MS = parseInt(process.env.ELECTRON_SPEC_UPDATER_RUN_BUDGET_MS || '', 10);
// How long a stopped run gets to unwind before its slot is retired.
const ABORT_GRACE_MS = 20000;
// How long stopRun waits for a killed process group to be gone, and then for
// the slot's update directories to become removable, before giving up on the
// slot. A SIGKILL'd app can hold files in its update directory open for a
// moment, and ShipIt (a launchd job, not our child) exits on its own schedule.
const KILL_WAIT_MS = 5000;
const CLEANUP_WAIT_MS = 5000;

// Squirrel derives its ShipIt launchd job, XPC name and cache dir from
// CFBundleIdentifier, so concurrent apps each need their own.
type Slot = {
  index: number;
  bundleId: string;
  shipItLabel: string;
  cacheDir: string;
  // Distinct package.json name per slot, so distinct userData dirs.
  nameSuffix: string;
  // Update zips carry the slot's bundle id, so they are cached per slot.
  zips: Record<string, string>;
};

const makeSlot = (index: number): Slot => {
  const bundleId = `com.github.Electron.spec${index}`;
  return {
    index,
    bundleId,
    shipItLabel: `${bundleId}.ShipIt`,
    cacheDir: path.join(os.homedir(), 'Library', 'Caches', `${bundleId}.ShipIt`),
    nameSuffix: `-spec${index}`,
    zips: {}
  };
};

class SlotPool {
  private free: Slot[];
  private waiters: ((slot: Slot) => void)[] = [];
  private nextIndex: number;
  // Slots minted for a retry. They are dropped, not pooled, when the retry
  // ends, so a retry never raises the concurrency for the rest of the file.
  private transient = new Set<Slot>();

  constructor(public readonly slots: Slot[]) {
    this.free = [...slots];
    this.nextIndex = slots.length;
  }

  /**
   * Replaces a slot whose run could not be stopped with a fresh one, so no
   * later run shares its bundle id, ShipIt job or cache dir.
   */
  retire(slot: Slot) {
    if (this.transient.delete(slot)) return;
    this.release(this.mint());
  }

  /**
   * A slot right now: a free one, or a fresh one. For a retry, which is the
   * test mocha is blocked on; queueing it behind lookahead runs for tests
   * that come later only adds their time to the failure.
   */
  acquireNow(): Slot {
    const slot = this.free.shift();
    if (slot) return slot;
    const fresh = this.mint();
    this.transient.add(fresh);
    return fresh;
  }

  private mint() {
    const fresh = makeSlot(this.nextIndex++);
    this.slots.push(fresh);
    return fresh;
  }

  acquire(): Promise<Slot> {
    const slot = this.free.shift();
    if (slot) return Promise.resolve(slot);
    return new Promise((resolve) => this.waiters.push(resolve));
  }

  release(slot: Slot) {
    if (this.transient.delete(slot)) return;
    const waiter = this.waiters.shift();
    if (waiter) waiter(slot);
    else this.free.push(slot);
  }
}

export type Mutation = {
  mutate: (appPath: string) => Promise<void>;
  mutationKey: string;
};

export type UpdatableAppOptions = {
  nextVersion: string;
  startFixture: string;
  endFixture: string;
  mutateAppPreSign?: Mutation;
  mutateAppPostSign?: Mutation;
};

// Per-run state for a pooled test, bound to one slot.
export type TaskContext = {
  /** Aborted when the run overruns its budget or the suite is finishing. */
  signal: AbortSignal;
  server: RoutedServer;
  port: number;
  requests: RoutedRequest[];
  launchApp: (appPath: string, args?: string[]) => Promise<{ code: number; out: string }>;
  launchAppSandboxed: (appPath: string, profilePath: string, args?: string[]) => Promise<{ code: number; out: string }>;
  spawnAppWithHandle: (appPath: string, args?: string[]) => cp.ChildProcess;
  /** Clone the signed template with `fixture` as its app, stamp the slot's bundle id, re-sign. */
  copySignedApp: (dir: string, fixture: string) => Promise<string>;
  /** As above for the start app, plus a cached update zip for `nextVersion`. */
  withUpdatableApp: (
    opts: UpdatableAppOptions,
    fn: (appPath: string, zipPath: string) => Promise<void>
  ) => Promise<void>;
  getUpdateZip: (version: string, fixture: string, pre?: Mutation, post?: Mutation) => Promise<string>;
  /** Serve `/update-check` pointing at `/update-file`, which serves whatever `pickZip` returns. */
  serveUpdate: (pickZip: string | (() => string)) => void;
  /** Resolves when the relaunched, updated app phones home. */
  relaunched: () => Promise<void>;
  getUpdateDirectoriesInCache: () => Promise<string[]>;
  cleanSquirrelCache: () => Promise<void>;
  getRunningShipIts: (appPath: string) => Promise<unknown[]>;
  /** Sets (or with `null`, deletes) a boolean in the slot app's NSUserDefaults domain. */
  setUserDefault: (key: string, value: boolean | null) => void;
};

type Task = {
  title: string;
  timeout: number;
  body: (ctx: TaskContext) => Promise<void>;
  run?: Promise<void>;
  // Bumped per run so a queued run can tell it was superseded.
  generation: number;
  started: boolean;
  // True once the mocha test has awaited a run, i.e. the next call is a retry.
  awaited: boolean;
  // Stops the current run.
  controller?: AbortController;
};

// What a run started, so that it can be stopped if it overruns.
type RunState = {
  signal: AbortSignal;
  children: Set<cp.ChildProcess>;
  // Every app is spawned as its own process group leader, so it and anything
  // it starts (helpers, ditto) can be killed together. ShipIt and the app it
  // relaunches are not in here: launchd spawns ShipIt, so they are found by
  // path instead (they all run from under an appPath).
  groups: Set<number>;
  // Fixture apps the run launched. The app, its ShipIt and the relaunched app
  // all run from these paths.
  appPaths: Set<string>;
  // What the run is doing, for the message when it overruns.
  phase: string;
};

export type UpdaterHarness = {
  /** The codesign identity the template was signed with. */
  identity: () => string;
  /** The stripped, deep-signed template app every fixture app is cloned from. */
  templateApp: () => string;
  launchApp: (appPath: string, args?: string[]) => Promise<{ code: number; out: string }>;
  shallowSign: (appPath: string) => Promise<void>;
  logOnError: (what: any, fn: () => void) => void;
  /**
   * A test whose body runs in a slot with its own update server, up to
   * CONCURRENCY at a time. Tasks start in declaration order, so keep nested
   * describes last (mocha runs them after the enclosing suite's own tests).
   */
  updaterIt: (title: string, body: (ctx: TaskContext) => Promise<void>, opts?: { timeout?: number }) => void;
};

/**
 * Sets up a spec file's share of the updater harness. Call it inside the
 * file's top-level `describe`: it registers the hooks that build the signed
 * template app before the tests and stop every run and clean up after them.
 */
export function setupUpdaterHarness(): UpdaterHarness {
  let identity = '';

  // Stripped and deep-signed once; every fixture app is an APFS clone of it
  // that only needs a shallow re-sign.
  let templateDir = '';
  let templateApp = '';
  const zipDirs: string[] = [];

  before(async function () {
    const result = getCodesignIdentity();
    if (result === null) return; // beforeEach below skips every test
    identity = result;

    this.timeout(5 * 60 * 1000);
    templateDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'electron-update-spec-template-'));
    templateApp = await copyMacOSFixtureApp(templateDir, null);
    stripFrameworkSymbols(templateApp);
    const signResult = await signApp(templateApp, identity);
    if (signResult.code !== 0) {
      throw new Error(`Failed to sign template app: ${signResult.out}`);
    }
  });

  beforeEach(function () {
    const result = getCodesignIdentity();
    if (result === null) {
      this.skip();
    } else {
      identity = result;
    }
  });

  const launchApp = (appPath: string, args: string[] = []) => {
    return spawn(path.resolve(appPath, 'Contents/MacOS/Electron'), args);
  };

  const spawnAppWithHandle = (appPath: string, args: string[] = []) => {
    return cp.spawn(path.resolve(appPath, 'Contents/MacOS/Electron'), args, { detached: true });
  };

  const logOnError = (what: any, fn: () => void) => {
    try {
      fn();
    } catch (err) {
      console.error(what);
      throw err;
    }
  };

  const shallowSign = async (appPath: string) => {
    const result = await signApp(appPath, identity, { deep: false });
    if (result.code !== 0) {
      throw new Error(`codesign failed for ${appPath}: ${result.out}`);
    }
  };

  const setBundleVersion = async (appPath: string, version: string) => {
    const appPJPath = path.resolve(appPath, 'Contents', 'Resources', 'app', 'package.json');
    await fs.promises.writeFile(appPJPath, (await fs.promises.readFile(appPJPath, 'utf8')).replace('1.0.0', version));
    const infoPath = path.resolve(appPath, 'Contents', 'Info.plist');
    await fs.promises.writeFile(
      infoPath,
      (await fs.promises.readFile(infoPath, 'utf8')).replace(
        /(<key>CFBundleShortVersionString<\/key>\s+<string>)[^<]+/g,
        `$1${version}`
      )
    );
  };

  const prepareApp = async (slot: Slot, dir: string, fixture: string, version: string, preSign?: Mutation) => {
    const appPath = await copyMacOSFixtureApp(dir, fixture, {
      sourceApp: templateApp,
      bundleId: slot.bundleId,
      appNameSuffix: slot.nameSuffix
    });
    await setBundleVersion(appPath, version);
    await preSign?.mutate(appPath);
    await shallowSign(appPath);
    return appPath;
  };

  const getUpdateZip = async (slot: Slot, version: string, fixture: string, pre?: Mutation, post?: Mutation) => {
    const key = `${version}-${fixture}-${pre?.mutationKey || 'no-pre-mutation'}-${post?.mutationKey || 'no-post-mutation'}`;
    if (!slot.zips[key]) {
      const dir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'electron-update-spec-zip-'));
      zipDirs.push(dir);
      const appPath = await prepareApp(slot, dir, fixture, version, pre);
      await post?.mutate(appPath);
      const zipPath = path.resolve(dir, 'update.zip');
      await spawn('zip', ['-0', '-r', '--symlinks', zipPath, './'], { cwd: dir });
      slot.zips[key] = zipPath;
    }
    return slot.zips[key];
  };

  const getUpdateDirectoriesInCache = async (slot: Slot) => {
    try {
      const entries = await fs.promises.readdir(slot.cacheDir, { withFileTypes: true });
      return entries
        .filter((entry) => entry.isDirectory() && entry.name.startsWith('update.'))
        .map((entry) => path.join(slot.cacheDir, entry.name));
    } catch {
      return [];
    }
  };

  const cleanSquirrelCache = async (slot: Slot) => {
    for (const dir of await getUpdateDirectoriesInCache(slot)) {
      await fs.promises.rm(dir, { recursive: true, force: true });
    }
  };

  // A directory a just-killed process still has files open in can briefly
  // refuse removal (ENOTEMPTY); keep trying for a while. Returns false rather
  // than throwing, so a cleanup failure never replaces a run's own error.
  const removeWithRetries = async (remove: () => Promise<void>, timeoutMs: number) => {
    const deadline = Date.now() + timeoutMs;
    while (true) {
      try {
        await remove();
        return true;
      } catch {
        if (Date.now() > deadline) return false;
        await delay(250);
      }
    }
  };

  const getRunningShipIts = async (slot: Slot, appPath: string) => {
    const processes = await psList();
    return processes.filter(
      (p) => p.cmd?.includes(`Squirrel.framework/Resources/ShipIt ${slot.shipItLabel}`) && p.cmd!.startsWith(appPath)
    );
  };

  const setUserDefault = (slot: Slot, key: string, value: boolean | null) => {
    // Both the app and ShipIt read the app's defaults domain.
    if (value === null) {
      cp.spawnSync('defaults', ['delete', slot.bundleId, key]);
    } else {
      cp.spawnSync('defaults', ['write', slot.bundleId, key, '-bool', value ? 'YES' : 'NO']);
    }
  };

  // `updaterIt` bodies run up to CONCURRENCY at a time, each in its own
  // slot with its own server; the mocha test just awaits its task.
  const pool = new SlotPool(Array.from({ length: CONCURRENCY }, (_, i) => makeSlot(i)));
  const tasks: Task[] = [];
  const inflight = new Set<Promise<void>>();
  let scheduled = false;
  let draining = false;

  const killGroup = (pgid: number) => {
    try {
      process.kill(-pgid, 'SIGKILL');
    } catch {
      // Already gone.
    }
  };

  const groupIsGone = (pgid: number) => {
    try {
      process.kill(-pgid, 0);
      return false;
    } catch {
      return true;
    }
  };

  // Kills every process running from under one of the prefixes, and the
  // given process groups, until nothing is left or the time is up. It loops
  // because a kill can be followed by a spawn: launchd starts ShipIt for a
  // job the app submitted just before it died, and a relaunched app submits
  // a job of its own. Returns false if something outlived the wait.
  const killEverything = async (groups: Iterable<number>, prefixes: string[], timeoutMs: number) => {
    const deadline = Date.now() + timeoutMs;
    while (true) {
      for (const pgid of groups) killGroup(pgid);
      const strays = (await psList()).filter((p) => p.cmd && prefixes.some((prefix) => p.cmd!.startsWith(prefix)));
      for (const p of strays) {
        try {
          process.kill(p.pid, 'SIGKILL');
        } catch {
          // Already gone.
        }
      }
      if (!strays.length && [...groups].every(groupIsGone)) return true;
      if (Date.now() > deadline) return false;
      await delay(100);
    }
  };

  const pathPrefixes = (paths: Iterable<string>) => [...paths].flatMap((p) => [p, `/private${p}`]);

  // What was going on when a run overran: the busiest processes, everything
  // running from the run's apps, and ShipIt's log for the slot. Printed once
  // per overrun so a slow runner can be told from a stuck one.
  const describeOverrun = async (slot: Slot, run: RunState) => {
    const lines: string[] = [];
    try {
      const prefixes = pathPrefixes(run.appPaths);
      const procs = await psList();
      const ours = procs.filter((p) => p.cmd && prefixes.some((prefix) => p.cmd!.startsWith(prefix)));
      const busiest = [...procs].sort((a, b) => (b.cpu ?? 0) - (a.cpu ?? 0)).slice(0, 8);
      const show = (p: (typeof procs)[number]) =>
        `    pid=${p.pid} cpu=${p.cpu ?? '?'}% mem=${p.memory ?? '?'}% ${(p.cmd ?? p.name).slice(0, 160)}`;
      lines.push('  busiest processes:', ...busiest.map(show));
      lines.push(`  processes from this run's apps (${ours.length}):`, ...ours.map(show));
    } catch (err) {
      lines.push(`  (could not list processes: ${err})`);
    }
    for (const name of ['ShipIt_stderr.log', 'ShipIt_stdout.log']) {
      try {
        const text = await fs.promises.readFile(path.join(slot.cacheDir, name), 'utf8');
        const tail = text.trimEnd().split('\n').slice(-25);
        lines.push(`  ${name} (last ${tail.length} lines):`, ...tail.map((l) => `    ${l.slice(0, 200)}`));
      } catch {
        lines.push(`  ${name}: none`);
      }
    }
    return lines.join('\n');
  };

  // Kills what a run left behind and clears its slot's ShipIt job and
  // downloaded updates, so the next run on the slot starts clean. Returns
  // false if the slot could not be cleaned, in which case the caller retires
  // it. Never throws: a cleanup error must not replace the run's own error.
  const stopRun = async (slot: Slot, run: RunState): Promise<boolean> => {
    for (const child of run.children) child.kill('SIGKILL');
    // ShipIt is a launchd job, and the app it relaunches is launchd's child
    // too, so neither is in a group of ours. Both run from under an appPath
    // (ShipIt from the bundle's Squirrel.framework), so kill by path as well;
    // removing the job alone does not reliably stop a ShipIt mid-install.
    cp.spawnSync('launchctl', ['remove', slot.shipItLabel]);
    // A killed ShipIt leaves its install-attempt count behind, and after
    // three it refuses to install at all on that label.
    cp.spawnSync('defaults', ['delete', slot.shipItLabel, 'SQRLShipItInstallationAttempts']);
    if (!(await killEverything(run.groups, pathPrefixes(run.appPaths), KILL_WAIT_MS))) return false;

    // The kill is delivered, but the files the app had open in its update
    // directory can take a moment to close, and ShipIt's exit is not ours to
    // observe, so removing the directories can briefly fail.
    return removeWithRetries(() => cleanSquirrelCache(slot), CLEANUP_WAIT_MS);
  };

  const runTask = async (task: Task, generation: number, { jumpQueue = false } = {}) => {
    const budget = RUN_BUDGET_OVERRIDE_MS > 0 ? RUN_BUDGET_OVERRIDE_MS : task.timeout * RUN_BUDGET_MULTIPLIER;
    // A run's budget only starts once it has a slot. A retry is the test
    // mocha is waiting on right now, so it gets one immediately rather than
    // queueing behind lookahead runs for tests that come later.
    const slot = jumpQueue ? pool.acquireNow() : await pool.acquire();
    if (draining || generation !== task.generation) {
      pool.release(slot);
      return;
    }

    const controller = new AbortController();
    task.controller = controller;
    task.started = true;

    const run: RunState = {
      signal: controller.signal,
      children: new Set(),
      groups: new Set(),
      appPaths: new Set(),
      phase: 'setup'
    };
    const stopped = new Promise<never>((resolve, reject) => {
      controller.signal.addEventListener('abort', () => reject(controller.signal.reason), { once: true });
    });
    stopped.catch(() => {});
    const timer = setTimeout(() => {
      controller.abort(
        new Error(`"${task.title}" ran out of its ${budget / 1000}s budget in slot ${slot.index} (${run.phase})`)
      );
    }, budget);

    const body = withTaskContext(slot, task.body, run);
    const settled = body.then(
      () => true,
      () => true
    );
    let retire = false;
    try {
      await Promise.race([body, stopped]);
    } catch (err) {
      if (controller.signal.aborted) {
        if (!draining) {
          console.log(
            `Overrun of "${task.title}" in slot ${slot.index} (${run.phase}):\n${await describeOverrun(slot, run)}`
          );
        }
        // Killing the run's processes rejects whatever the body is waiting on.
        await stopRun(slot, run);
        retire = !(await Promise.race([settled, delay(ABORT_GRACE_MS).then(() => false)]));
      }
      // After any failure, clean up what the run left. After an abort this
      // also catches anything the body started while unwinding. If the slot
      // cannot be cleaned, retire it rather than hand a dirty one to the
      // next run.
      if (!(await stopRun(slot, run))) retire = true;
      throw err;
    } finally {
      clearTimeout(timer);
      if (task.controller === controller) task.controller = undefined;
      if (retire) {
        pool.retire(slot);
      } else {
        pool.release(slot);
      }
    }
  };

  const startTask = (task: Task, opts?: { jumpQueue?: boolean }) => {
    task.started = false;
    const run = runTask(task, ++task.generation, opts);
    task.run = run;
    inflight.add(run);
    // The test may not be awaiting yet; avoid an unhandled rejection.
    run.catch(() => {}).finally(() => inflight.delete(run));
    return run;
  };

  const scheduleFrom = (index: number) => {
    if (scheduled || CONCURRENCY === 1) return;
    scheduled = true;
    for (let i = index; i < tasks.length; i++) {
      if (!tasks[i].run) startTask(tasks[i]);
    }
  };

  const updaterIt = (title: string, body: (ctx: TaskContext) => Promise<void>, { timeout = 120000 } = {}) => {
    const task: Task = { title, timeout, body, generation: 0, started: false, awaited: false };
    const index = tasks.push(task) - 1;
    it(title, async function () {
      // Each run enforces its own budget from when it gets a slot, so this is
      // only a backstop in case the pool stops making progress.
      this.timeout(30 * 60 * 1000);
      scheduleFrom(index);
      // Run now, ahead of the queue, if there is no lookahead run, this is a
      // retry, or --grep left ours queued behind tests that never ran.
      if (!task.run || task.awaited || !task.started) startTask(task, { jumpQueue: true });
      task.awaited = true;
      await task.run;
    });
  };

  // Registered before the template cleanup below, so it runs first.
  after(async function () {
    // With --grep, lookahead runs for tests that never executed may still be
    // going; stop them, and make queued ones bail.
    draining = true;
    this.timeout(10 * 60 * 1000);
    for (const task of tasks) task.controller?.abort(new Error('The suite finished before this run did'));
    await Promise.allSettled([...inflight]);
    // A stop that gave up on a slot (and retired it) may have left something
    // running; every fixture app of this suite lives under this prefix.
    for (const slot of pool.slots) cp.spawnSync('launchctl', ['remove', slot.shipItLabel]);
    await killEverything([], pathPrefixes([path.resolve(os.tmpdir(), 'electron-update-spec-')]), KILL_WAIT_MS);
    for (const slot of pool.slots) {
      cp.spawnSync('defaults', ['delete', slot.bundleId]);
      cp.spawnSync('defaults', ['delete', slot.shipItLabel, 'SQRLShipItInstallationAttempts']);
      // Runs aborted just above may still be releasing their files.
      await removeWithRetries(() => fs.promises.rm(slot.cacheDir, { recursive: true, force: true }), CLEANUP_WAIT_MS);
    }
  });

  after(async () => {
    for (const dir of zipDirs) {
      cp.spawnSync('rm', ['-r', dir]);
    }
    if (templateDir) cp.spawnSync('rm', ['-r', templateDir]);
  });

  // Like spawn() from codesign-helpers, but records the child so an
  // overrunning run can kill it.
  const spawnForRun = (run: RunState, cmd: string, args: string[]) => {
    // A body unwinding from an abort must not start anything the stop
    // already ran past.
    if (run.signal.aborted) throw run.signal.reason;
    let out = '';
    const child = cp.spawn(cmd, args, { detached: true });
    run.children.add(child);
    if (child.pid) run.groups.add(child.pid);
    child.stdout.on('data', (chunk: Buffer) => {
      out += chunk.toString();
    });
    child.stderr.on('data', (chunk: Buffer) => {
      out += chunk.toString();
    });
    return new Promise<{ code: number; out: string }>((resolve, reject) => {
      child.on('error', reject);
      child.on('exit', (code, signal) => {
        run.children.delete(child);
        if (signal) {
          reject(new Error(`${path.basename(cmd)} was killed by ${signal}`));
        } else {
          resolve({ code: code!, out });
        }
      });
    });
  };

  const withTaskContext = async (slot: Slot, body: (ctx: TaskContext) => Promise<void>, run: RunState) => {
    const requests: RoutedRequest[] = [];
    const server = createRoutedServer();
    server.use((req, res, next) => {
      requests.push(req);
      next();
    });
    const httpServer = await new Promise<http.Server>((resolve) => {
      const s = server.listen(0, '127.0.0.1', () => resolve(s));
    });
    const port = (httpServer.address() as AddressInfo).port;

    const launchForRun = async (appPath: string, args: string[] = []) => {
      run.appPaths.add(appPath);
      run.phase = 'waiting for the app to exit';
      const result = await spawnForRun(run, path.resolve(appPath, 'Contents/MacOS/Electron'), args);
      run.phase = 'after the app exited, such as waiting for a relaunch';
      return result;
    };

    const ctx: TaskContext = {
      signal: run.signal,
      server,
      port,
      requests,
      launchApp: launchForRun,
      launchAppSandboxed: (appPath, profilePath, args = []) => {
        run.appPaths.add(appPath);
        run.phase = 'waiting for the sandboxed app to exit';
        return spawnForRun(run, '/usr/bin/sandbox-exec', [
          '-f',
          profilePath,
          path.resolve(appPath, 'Contents/MacOS/Electron'),
          ...args,
          '--no-sandbox'
        ]);
      },
      spawnAppWithHandle: (appPath, args = []) => {
        if (run.signal.aborted) throw run.signal.reason;
        run.appPaths.add(appPath);
        const child = spawnAppWithHandle(appPath, args);
        run.children.add(child);
        if (child.pid) run.groups.add(child.pid);
        child.on('exit', () => run.children.delete(child));
        return child;
      },
      copySignedApp: async (dir, fixture) => {
        run.phase = 'preparing the app';
        const appPath = await prepareApp(slot, dir, fixture, '1.0.0');
        run.appPaths.add(appPath);
        return appPath;
      },
      getUpdateZip: (version, fixture, pre, post) => getUpdateZip(slot, version, fixture, pre, post),
      withUpdatableApp: async (opts, fn) => {
        await withTempDirectory(async (dir) => {
          run.phase = 'preparing the app and its update';
          const appPath = await prepareApp(slot, dir, opts.startFixture, '1.0.0', opts.mutateAppPreSign);
          run.appPaths.add(appPath);
          const zipPath = await getUpdateZip(
            slot,
            opts.nextVersion,
            opts.endFixture,
            opts.mutateAppPreSign,
            opts.mutateAppPostSign
          );
          await fn(appPath, zipPath);
        });
      },
      serveUpdate: (pickZip) => {
        server.get('/update-file', (req, res) => {
          res.download(typeof pickZip === 'string' ? pickZip : pickZip());
        });
        server.get('/update-check', (req, res) => {
          res.json({
            url: `http://localhost:${port}/update-file`,
            name: 'My Release Name',
            notes: 'Theses are some release notes innit',
            pub_date: new Date().toString()
          });
        });
      },
      relaunched: () => {
        const relaunch = new Promise<void>((resolve, reject) => {
          server.get('/update-check/updated/:version', (req, res) => {
            res.status(204).send();
            resolve();
          });
          // Otherwise a relaunch that never comes keeps the run going forever.
          run.signal.addEventListener('abort', () => reject(run.signal.reason), { once: true });
        });
        // Tests create this before launching the app and await it later.
        relaunch.catch(() => {});
        return relaunch;
      },
      getUpdateDirectoriesInCache: () => getUpdateDirectoriesInCache(slot),
      cleanSquirrelCache: () => cleanSquirrelCache(slot),
      getRunningShipIts: (appPath) => getRunningShipIts(slot, appPath),
      setUserDefault: (key, value) => setUserDefault(slot, key, value)
    };

    try {
      await body(ctx);
    } finally {
      // A killed app can leave a keep-alive connection open, and close()
      // waits for those.
      if (run.signal.aborted) httpServer.closeAllConnections();
      await new Promise<void>((resolve) => httpServer.close(() => resolve()));
    }
  };

  return {
    identity: () => identity,
    templateApp: () => templateApp,
    launchApp,
    shallowSign,
    logOnError,
    updaterIt
  };
}
