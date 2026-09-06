import { autoUpdater } from 'electron';

import { expect } from 'chai';
import psList from 'ps-list';

import * as cp from 'node:child_process';
import { randomUUID } from 'node:crypto';
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
  stripFrameworkSymbols,
  unsignApp
} from './lib/codesign-helpers';
import { withTempDirectory } from './lib/fs-helpers';
import { createRoutedServer, RoutedRequest, RoutedResponse, RoutedServer } from './lib/http-server-helpers';
import { ifdescribe, ifit } from './lib/spec-helpers';

// How many fixture apps may be updating at once; each is about a core of
// codesign/ditto/ShipIt work. Set to 1 to run every test's work inline.
const CONCURRENCY = (() => {
  const fromEnv = parseInt(process.env.ELECTRON_SPEC_UPDATER_CONCURRENCY || '', 10);
  if (fromEnv > 0) return fromEnv;
  // The Intel CI runners do this work several times slower than Apple silicon,
  // and at 4 their slowest run came within 15% of its budget.
  if (process.arch === 'x64') return 2;
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

  constructor(public readonly slots: Slot[]) {
    this.free = [...slots];
    this.nextIndex = slots.length;
  }

  /**
   * Replaces a slot whose run could not be stopped with a fresh one, so no
   * later run shares its bundle id, ShipIt job or cache dir.
   */
  retire() {
    const fresh = makeSlot(this.nextIndex++);
    this.slots.push(fresh);
    this.release(fresh);
  }

  acquire({ jumpQueue = false } = {}): Promise<Slot> {
    const slot = this.free.shift();
    if (slot) return Promise.resolve(slot);
    return new Promise((resolve) => (jumpQueue ? this.waiters.unshift(resolve) : this.waiters.push(resolve)));
  }

  release(slot: Slot) {
    const waiter = this.waiters.shift();
    if (waiter) waiter(slot);
    else this.free.push(slot);
  }
}

type Mutation = {
  mutate: (appPath: string) => Promise<void>;
  mutationKey: string;
};

type UpdatableAppOptions = {
  nextVersion: string;
  startFixture: string;
  endFixture: string;
  mutateAppPreSign?: Mutation;
  mutateAppPostSign?: Mutation;
};

// Per-run state for a pooled test, bound to one slot.
type TaskContext = {
  slot: Slot;
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

// We can only test the auto updater on darwin non-component builds
ifdescribe(shouldRunCodesignTests && !process.env.IS_UBSAN)('autoUpdater behavior', function () {
  this.timeout(120000);

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

  it('should have a valid code signing identity', () => {
    expect(identity).to.be.a('string').with.lengthOf.at.least(1);
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

  after(async () => {
    for (const dir of zipDirs) {
      cp.spawnSync('rm', ['-r', dir]);
    }
    if (templateDir) cp.spawnSync('rm', ['-r', templateDir]);
  });

  // These use the raw build output, not the template. On arm64 builds the
  // built app is self-signed by default so the setFeedURL call always works.
  ifit(process.arch !== 'arm64')('should fail to set the feed URL when the app is not signed', async () => {
    await withTempDirectory(async (dir) => {
      const appPath = await copyMacOSFixtureApp(dir);
      await unsignApp(appPath);
      const launchResult = await launchApp(appPath, ['http://myupdate']);
      console.log(launchResult);
      expect(launchResult.code).to.equal(1);
      expect(launchResult.out).to.include('Could not get code signature for running application');
    });
  });

  ifit(process.arch !== 'arm64')(
    'should fail with code signature error when serverType is default and app is unsigned',
    async () => {
      await withTempDirectory(async (dir) => {
        const appPath = await copyMacOSFixtureApp(dir);
        await unsignApp(appPath);
        const launchResult = await launchApp(appPath, ['', 'default']);
        expect(launchResult.code).to.equal(1);
        expect(launchResult.out).to.include('Could not get code signature for running application');
      });
    }
  );

  ifit(process.arch !== 'arm64')(
    'should fail with code signature error when serverType is json and app is unsigned',
    async () => {
      await withTempDirectory(async (dir) => {
        const appPath = await copyMacOSFixtureApp(dir);
        await unsignApp(appPath);
        const launchResult = await launchApp(appPath, ['', 'json']);
        expect(launchResult.code).to.equal(1);
        expect(launchResult.out).to.include('Could not get code signature for running application');
      });
    }
  );

  ifit(process.arch !== 'arm64')(
    'should fail with serverType error when an invalid serverType is provided',
    async () => {
      await withTempDirectory(async (dir) => {
        const appPath = await copyMacOSFixtureApp(dir);
        const launchResult = await launchApp(appPath, ['', 'weow']);
        expect(launchResult.code).to.equal(1);
        expect(launchResult.out).to.include("Expected serverType to be 'default' or 'json'");
      });
    }
  );

  it('should cleanly set the feed URL when the app is signed', async () => {
    await withTempDirectory(async (dir) => {
      const appPath = await copyMacOSFixtureApp(dir, 'initial', { sourceApp: templateApp });
      await shallowSign(appPath);
      const launchResult = await launchApp(appPath, ['http://myupdate']);
      expect(launchResult.code).to.equal(0);
      expect(launchResult.out).to.include('Feed URL Set: http://myupdate');
    });
  });

  describe('with update server', () => {
    // `updaterIt` bodies run up to CONCURRENCY at a time, each in its own
    // slot with its own server; the mocha test just awaits its task. Tasks
    // start in declaration order, so keep nested describes last (mocha runs
    // them after this suite's own tests).
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
      // A run's budget only starts once it has a slot. A retry, though, is the
      // test mocha is waiting on right now; if every slot is held by a lookahead
      // run that is itself stuck, waiting for one is unbounded and silent. Give
      // it the budget to get a slot, then make one.
      let slot: Slot;
      if (jumpQueue) {
        const acquired = pool.acquire({ jumpQueue });
        const timeout = delay(budget).then(() => null);
        const first = await Promise.race([acquired, timeout]);
        if (first) {
          slot = first;
        } else {
          pool.retire();
          slot = await acquired;
        }
      } else {
        slot = await pool.acquire({ jumpQueue });
      }
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
          pool.retire();
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
        slot,
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

    updaterIt('should hit the update endpoint when checkForUpdates is called', async (ctx) => {
      await withTempDirectory(async (dir) => {
        const appPath = await ctx.copySignedApp(dir, 'check');
        ctx.server.get('/update-check', (req, res) => {
          res.status(204).send();
        });
        const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
        logOnError(launchResult, () => {
          expect(launchResult.code).to.equal(0);
          expect(ctx.requests).to.have.lengthOf(1);
          expect(ctx.requests[0]).to.have.property('url', '/update-check');
          expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
        });
      });
    });

    updaterIt('should hit the update endpoint with customer headers when checkForUpdates is called', async (ctx) => {
      await withTempDirectory(async (dir) => {
        const appPath = await ctx.copySignedApp(dir, 'check-with-headers');
        ctx.server.get('/update-check', (req, res) => {
          res.status(204).send();
        });
        const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
        logOnError(launchResult, () => {
          expect(launchResult.code).to.equal(0);
          expect(ctx.requests).to.have.lengthOf(1);
          expect(ctx.requests[0]).to.have.property('url', '/update-check');
          expect(ctx.requests[0].header('x-test')).to.equal('this-is-a-test');
        });
      });
    });

    updaterIt(
      'should hit the download endpoint when an update is available and error if the file is bad',
      async (ctx) => {
        await withTempDirectory(async (dir) => {
          const appPath = await ctx.copySignedApp(dir, 'update');
          ctx.server.get('/update-file', (req, res) => {
            res.status(500).send('This is not a file');
          });
          ctx.server.get('/update-check', (req, res) => {
            res.json({
              url: `http://localhost:${ctx.port}/update-file`,
              name: 'My Release Name',
              notes: 'Theses are some release notes innit',
              pub_date: new Date().toString()
            });
          });
          const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
          logOnError(launchResult, () => {
            expect(launchResult).to.have.property('code', 1);
            expect(launchResult.out).to.include('Update download failed. The server sent an invalid response.');
            expect(ctx.requests).to.have.lengthOf(2);
            expect(ctx.requests[0]).to.have.property('url', '/update-check');
            expect(ctx.requests[1]).to.have.property('url', '/update-file');
            expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
            expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
          });
        });
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and update successfully when the zip is provided',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update',
            endFixture: 'update'
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const relaunchPromise = ctx.relaunched();
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });

            await relaunchPromise;
            expect(ctx.requests).to.have.lengthOf(3);
            expect(ctx.requests[2].url).to.equal('/update-check/updated/2.0.0');
            expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and update successfully when the zip is provided even after a different update was staged',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update-stack',
            endFixture: 'update-stack'
          },
          async (appPath, updateZipPath2) => {
            const updateZipPath3 = await ctx.getUpdateZip('3.0.0', 'update-stack');
            let updateCount = 0;
            ctx.server.get('/update-file', (req, res) => {
              res.download(updateCount > 1 ? updateZipPath3 : updateZipPath2);
            });
            ctx.server.get('/update-check', (req, res) => {
              updateCount++;
              res.json({
                url: `http://localhost:${ctx.port}/update-file`,
                name: 'My Release Name',
                notes: 'Theses are some release notes innit',
                pub_date: new Date().toString()
              });
            });
            const relaunchPromise = ctx.relaunched();
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
              expect(ctx.requests).to.have.lengthOf(4);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[2]).to.have.property('url', '/update-check');
              expect(ctx.requests[3]).to.have.property('url', '/update-file');
              expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[3].header('user-agent')).to.include('Electron/');
            });

            await relaunchPromise;
            expect(ctx.requests).to.have.lengthOf(5);
            expect(ctx.requests[4].url).to.equal('/update-check/updated/3.0.0');
            expect(ctx.requests[4].header('user-agent')).to.include('Electron/');
          }
        );
      },
      { timeout: 180000 }
    );

    updaterIt(
      'should preserve the staged update directory and prune orphaned ones when a new update is downloaded',
      async (ctx) => {
        // Clean up any existing update directories before the test
        await ctx.cleanSquirrelCache();

        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update-stack',
            endFixture: 'update-stack'
          },
          async (appPath, updateZipPath2) => {
            const updateZipPath3 = await ctx.getUpdateZip('3.0.0', 'update-stack');
            let updateCount = 0;
            let downloadCount = 0;
            let dirsDuringFirstDownload: string[] = [];
            let dirsDuringSecondDownload: string[] = [];

            ctx.server.get('/update-file', async (req, res) => {
              downloadCount++;
              // Snapshot update directories at the moment each download begins.
              // By this point uniqueTemporaryDirectoryForUpdate has already run
              // (prune + mkdtemp). We want to verify:
              //   1st download: 1 dir (nothing to preserve, nothing to prune)
              //   2nd download: 2 dirs (staged dir from 1st check is preserved
              //                 so quitAndInstall stays safe, + new temp dir)
              // The count never exceeds 2 across repeated checks — orphaned dirs
              // (no longer referenced by ShipItState.plist) get pruned.
              if (downloadCount === 1) {
                dirsDuringFirstDownload = await ctx.getUpdateDirectoriesInCache();
              } else if (downloadCount === 2) {
                dirsDuringSecondDownload = await ctx.getUpdateDirectoriesInCache();
              }
              res.download(updateCount > 1 ? updateZipPath3 : updateZipPath2);
            });
            ctx.server.get('/update-check', (req, res) => {
              updateCount++;
              res.json({
                url: `http://localhost:${ctx.port}/update-file`,
                name: 'My Release Name',
                notes: 'Theses are some release notes innit',
                pub_date: new Date().toString()
              });
            });
            const relaunchPromise = ctx.relaunched();
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
            });

            await relaunchPromise;

            // First download: exactly one temp dir (the first update).
            expect(dirsDuringFirstDownload).to.have.lengthOf(
              1,
              `Expected 1 update directory during first download but found ${dirsDuringFirstDownload.length}: ${dirsDuringFirstDownload.join(', ')}`
            );

            // Second download: exactly two — the staged one preserved + the new
            // one. Crucially the first download's directory must still be present,
            // otherwise a mid-download quitAndInstall would find a dangling
            // ShipItState.plist.
            expect(dirsDuringSecondDownload).to.have.lengthOf(
              2,
              `Expected 2 update directories during second download (staged + new) but found ${dirsDuringSecondDownload.length}: ${dirsDuringSecondDownload.join(', ')}`
            );
            expect(dirsDuringSecondDownload).to.include(
              dirsDuringFirstDownload[0],
              'The staged update directory from the first download must be preserved during the second download'
            );
          }
        );
      }
    );

    updaterIt(
      'should keep the update directory count bounded across repeated checks',
      async (ctx) => {
        // Verifies the orphan prune actually fires: after a second download
        // completes and rewrites ShipItState.plist, the first directory is no
        // longer referenced and must be removed when a third check begins.
        // Without this, directories would accumulate forever.
        await ctx.cleanSquirrelCache();

        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update-triple-stack',
            endFixture: 'update-triple-stack'
          },
          async (appPath, updateZipPath2) => {
            const updateZipPath3 = await ctx.getUpdateZip('3.0.0', 'update-triple-stack');
            const updateZipPath4 = await ctx.getUpdateZip('4.0.0', 'update-triple-stack');
            let downloadCount = 0;
            const dirsPerDownload: string[][] = [];

            ctx.server.get('/update-file', async (req, res) => {
              downloadCount++;
              // Snapshot after prune+mkdtemp but before the payload transfers.
              dirsPerDownload.push(await ctx.getUpdateDirectoriesInCache());
              const zips = [updateZipPath2, updateZipPath3, updateZipPath4];
              res.download(zips[Math.min(downloadCount, zips.length) - 1]);
            });
            ctx.server.get('/update-check', (req, res) => {
              res.json({
                url: `http://localhost:${ctx.port}/update-file`,
                name: 'My Release Name',
                notes: 'Theses are some release notes innit',
                pub_date: new Date().toString()
              });
            });
            const relaunchPromise = ctx.relaunched();

            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
            });

            await relaunchPromise;
            expect(ctx.requests[ctx.requests.length - 1].url).to.equal('/update-check/updated/4.0.0');

            expect(dirsPerDownload).to.have.lengthOf(3);

            // 1st: fresh cache, 1 dir.
            expect(dirsPerDownload[0]).to.have.lengthOf(1, `1st download: ${dirsPerDownload[0].join(', ')}`);

            // 2nd: staged (1st) preserved + new = 2 dirs.
            expect(dirsPerDownload[1]).to.have.lengthOf(2, `2nd download: ${dirsPerDownload[1].join(', ')}`);
            expect(dirsPerDownload[1]).to.include(dirsPerDownload[0][0]);

            // 3rd: 1st is now orphaned (plist points to 2nd) — must be pruned.
            // Staged (2nd) preserved + new = still 2 dirs. Bounded.
            expect(dirsPerDownload[2]).to.have.lengthOf(2, `3rd download: ${dirsPerDownload[2].join(', ')}`);
            expect(dirsPerDownload[2]).to.not.include(
              dirsPerDownload[0][0],
              'The first (now orphaned) update directory must be pruned on the third check'
            );
            const secondDir = dirsPerDownload[1].find((d) => d !== dirsPerDownload[0][0]);
            expect(dirsPerDownload[2]).to.include(
              secondDir,
              'The second (currently staged) update directory must be preserved on the third check'
            );
          }
        );
      },
      { timeout: 240000 }
    );

    // Regression test for https://github.com/electron/electron/issues/50200
    //
    // When checkForUpdates() is called again after an update has been staged,
    // Squirrel creates a new temporary directory and prunes old ones. If the
    // prune removes the directory that ShipItState.plist references while the
    // second download is still in flight, a subsequent quitAndInstall() will
    // fail with ENOENT and the app will never relaunch.
    updaterIt(
      'should install the staged update when quitAndInstall is called while a second check is in flight',
      async (ctx) => {
        await ctx.cleanSquirrelCache();

        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update-race',
            endFixture: 'update-race'
          },
          async (appPath, updateZipPath) => {
            let downloadCount = 0;
            let stalledResponse: RoutedResponse | null = null;

            ctx.server.get('/update-file', (req, res) => {
              downloadCount++;
              if (downloadCount === 1) {
                // First download completes normally and stages the update.
                res.download(updateZipPath);
              } else {
                // Second download: stall indefinitely to simulate a slow
                // network. This keeps the second check "in progress" when
                // quitAndInstall() fires. Hold onto the response so we can
                // clean it up later.
                stalledResponse = res;
              }
            });
            ctx.server.get('/update-check', (req, res) => {
              res.json({
                url: `http://localhost:${ctx.port}/update-file`,
                name: 'My Release Name',
                notes: 'Theses are some release notes innit',
                pub_date: new Date().toString()
              });
            });
            const relaunchPromise = ctx.relaunched();

            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
              expect(launchResult.out).to.include('Calling quitAndInstall mid-download');
              // First check + first download + second check + stalled second download.
              expect(ctx.requests).to.have.lengthOf(4);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[2]).to.have.property('url', '/update-check');
              expect(ctx.requests[3]).to.have.property('url', '/update-file');
              // The second download must have been in flight (never completed)
              // when quitAndInstall was called.
              expect(launchResult.out).to.not.include('Unexpected second download completion');
            });

            // Unblock the stalled response now that the initial app has exited
            // so the server can shut down cleanly.
            if (stalledResponse) {
              (stalledResponse as RoutedResponse).status(500).end();
            }

            // The originally staged update (2.0.0) must have been applied and
            // the app must relaunch, proving the staged update directory was
            // not pruned out from under ShipItState.plist.
            await relaunchPromise;
            expect(ctx.requests).to.have.lengthOf(5);
            expect(ctx.requests[4].url).to.equal('/update-check/updated/2.0.0');
            expect(ctx.requests[4].header('user-agent')).to.include('Electron/');
          }
        );
      }
    );

    updaterIt('should update to lower version numbers', async (ctx) => {
      await ctx.withUpdatableApp(
        {
          nextVersion: '0.0.1',
          startFixture: 'update',
          endFixture: 'update'
        },
        async (appPath, updateZipPath) => {
          ctx.serveUpdate(updateZipPath);
          const relaunchPromise = ctx.relaunched();
          const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
          logOnError(launchResult, () => {
            expect(launchResult).to.have.property('code', 0);
            expect(launchResult.out).to.include('Update Downloaded');
            expect(ctx.requests).to.have.lengthOf(2);
            expect(ctx.requests[0]).to.have.property('url', '/update-check');
            expect(ctx.requests[1]).to.have.property('url', '/update-file');
            expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
            expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
          });

          await relaunchPromise;
          expect(ctx.requests).to.have.lengthOf(3);
          expect(ctx.requests[2].url).to.equal('/update-check/updated/0.0.1');
          expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
        }
      );
    });

    updaterIt('should abort the update if the application is still running when ShipIt kicks off', async (ctx) => {
      await ctx.withUpdatableApp(
        {
          nextVersion: '2.0.0',
          startFixture: 'update',
          endFixture: 'update'
        },
        async (appPath, updateZipPath) => {
          ctx.serveUpdate(updateZipPath);

          enum FlipFlop {
            INITIAL,
            FLIPPED,
            FLOPPED
          }

          const shipItFlipFlopPromise = new Promise<void>((resolve) => {
            let state = FlipFlop.INITIAL;
            const checker = setInterval(async () => {
              const running = await ctx.getRunningShipIts(appPath);
              switch (state) {
                case FlipFlop.INITIAL: {
                  if (running.length) state = FlipFlop.FLIPPED;
                  break;
                }
                case FlipFlop.FLIPPED: {
                  if (!running.length) state = FlipFlop.FLOPPED;
                  break;
                }
              }
              if (state === FlipFlop.FLOPPED) {
                clearInterval(checker);
                resolve();
              }
            }, 500);
          });

          const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
          const retainerHandle = ctx.spawnAppWithHandle(appPath, ['remain-open']);
          try {
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });

            await shipItFlipFlopPromise;
            expect(ctx.requests).to.have.lengthOf(2, 'should not have relaunched the updated app');
            expect(
              JSON.parse(
                await fs.promises.readFile(path.resolve(appPath, 'Contents/Resources/app/package.json'), 'utf8')
              ).version
            ).to.equal('1.0.0', 'should still be the old version on disk');
          } finally {
            retainerHandle.kill('SIGINT');
          }
        }
      );
    });

    updaterIt(
      'should hit the download endpoint when an update is available and fail when the zip signature is invalid',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update',
            endFixture: 'update',
            mutateAppPostSign: {
              mutationKey: 'add-resource',
              mutate: async (appPath) => {
                const resourcesPath = path.resolve(appPath, 'Contents', 'Resources', 'app', 'injected.txt');
                await fs.promises.writeFile(resourcesPath, 'demo');
              }
            }
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Code signature at URL');
              expect(launchResult.out).to.include('a sealed resource is missing or invalid');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and fail when the ShipIt binary is a symlink',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update',
            endFixture: 'update',
            mutateAppPostSign: {
              mutationKey: 'modify-shipit',
              mutate: async (appPath) => {
                const shipItPath = path.resolve(
                  appPath,
                  'Contents',
                  'Frameworks',
                  'Squirrel.framework',
                  'Resources',
                  'ShipIt'
                );
                await fs.promises.rm(shipItPath, { force: true, recursive: true });
                await fs.promises.symlink('/tmp/ShipIt', shipItPath, 'file');
              }
            }
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Code signature at URL');
              expect(launchResult.out).to.include('a sealed resource is missing or invalid');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and fail when the Electron Framework is modified',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update',
            endFixture: 'update',
            mutateAppPostSign: {
              mutationKey: 'modify-eframework',
              mutate: async (appPath) => {
                const shipItPath = path.resolve(
                  appPath,
                  'Contents',
                  'Frameworks',
                  'Electron Framework.framework',
                  'Electron Framework'
                );
                await fs.promises.appendFile(shipItPath, Buffer.from('123'));
              }
            }
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Code signature at URL');
              expect(launchResult.out).to.include(' main executable failed strict validation');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and fail when the zip extraction process fails to launch',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update',
            endFixture: 'update'
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchAppSandboxed(
              appPath,
              path.resolve(__dirname, 'fixtures/auto-update/sandbox/block-ditto.sb'),
              [`http://localhost:${ctx.port}/update-check`]
            );
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Starting ditto task failed with error:');
              expect(launchResult.out).to.include('SQRLZipArchiverErrorDomain');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and update successfully when the zip is provided with JSON update mode',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0',
            startFixture: 'update-json',
            endFixture: 'update-json'
          },
          async (appPath, updateZipPath) => {
            ctx.server.get('/update-file', (req, res) => {
              res.download(updateZipPath);
            });
            ctx.server.get('/update-check', (req, res) => {
              res.json({
                currentRelease: '2.0.0',
                releases: [
                  {
                    version: '2.0.0',
                    updateTo: {
                      version: '2.0.0',
                      url: `http://localhost:${ctx.port}/update-file`,
                      name: 'My Release Name',
                      notes: 'Theses are some release notes innit',
                      pub_date: new Date().toString()
                    }
                  }
                ]
              });
            });
            const relaunchPromise = ctx.relaunched();
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });

            await relaunchPromise;
            expect(ctx.requests).to.have.lengthOf(3);
            expect(ctx.requests[2]).to.have.property('url', '/update-check/updated/2.0.0');
            expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
          }
        );
      }
    );

    updaterIt(
      'should hit the download endpoint when an update is available and not update in JSON update mode when the currentRelease is older than the current version',
      async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '0.1.0',
            startFixture: 'update-json',
            endFixture: 'update-json'
          },
          async (appPath, updateZipPath) => {
            ctx.server.get('/update-file', (req, res) => {
              res.download(updateZipPath);
            });
            ctx.server.get('/update-check', (req, res) => {
              res.json({
                currentRelease: '0.1.0',
                releases: [
                  {
                    version: '0.1.0',
                    updateTo: {
                      version: '0.1.0',
                      url: `http://localhost:${ctx.port}/update-file`,
                      name: 'My Release Name',
                      notes: 'Theses are some release notes innit',
                      pub_date: new Date().toString()
                    }
                  }
                ]
              });
            });
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('No update available');
              expect(ctx.requests).to.have.lengthOf(1);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
            });
          }
        );
      }
    );

    // Nested describes go last; see the note at the top of this block.

    describe('with ElectronSquirrelPreventDowngrades enabled', () => {
      const preventDowngrades: Mutation = {
        mutationKey: 'prevent-downgrades',
        mutate: async (appPath) => {
          const infoPath = path.resolve(appPath, 'Contents', 'Info.plist');
          await fs.promises.writeFile(
            infoPath,
            (await fs.promises.readFile(infoPath, 'utf8')).replace(
              '<key>NSSupportsAutomaticGraphicsSwitching</key>',
              '<key>ElectronSquirrelPreventDowngrades</key><true/><key>NSSupportsAutomaticGraphicsSwitching</key>'
            )
          );
        }
      };

      updaterIt('should not update to lower version numbers', async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '0.0.1',
            startFixture: 'update',
            endFixture: 'update',
            mutateAppPreSign: preventDowngrades
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Cannot update to a bundle with a lower version number');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      });

      updaterIt('should not update to version strings that are not simple Major.Minor.Patch', async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '2.0.0-bad',
            startFixture: 'update',
            endFixture: 'update',
            mutateAppPreSign: preventDowngrades
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 1);
              expect(launchResult.out).to.include('Cannot update to a bundle with a lower version number');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });
          }
        );
      });

      updaterIt('should still update to higher version numbers', async (ctx) => {
        await ctx.withUpdatableApp(
          {
            nextVersion: '1.0.1',
            startFixture: 'update',
            endFixture: 'update'
          },
          async (appPath, updateZipPath) => {
            ctx.serveUpdate(updateZipPath);
            const relaunchPromise = ctx.relaunched();
            const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
            logOnError(launchResult, () => {
              expect(launchResult).to.have.property('code', 0);
              expect(launchResult.out).to.include('Update Downloaded');
              expect(ctx.requests).to.have.lengthOf(2);
              expect(ctx.requests[0]).to.have.property('url', '/update-check');
              expect(ctx.requests[1]).to.have.property('url', '/update-file');
              expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
              expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
            });

            await relaunchPromise;
            expect(ctx.requests).to.have.lengthOf(3);
            expect(ctx.requests[2].url).to.equal('/update-check/updated/1.0.1');
            expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
          }
        );
      });

      it('should compare version numbers correctly', () => {
        expect(autoUpdater.isVersionAllowedForUpdate!('1.0.0', '2.0.0')).to.equal(true);
        expect(autoUpdater.isVersionAllowedForUpdate!('1.0.1', '1.0.10')).to.equal(true);
        expect(autoUpdater.isVersionAllowedForUpdate!('1.0.10', '1.0.1')).to.equal(false);
        expect(autoUpdater.isVersionAllowedForUpdate!('1.31.1', '1.32.0')).to.equal(true);
        expect(autoUpdater.isVersionAllowedForUpdate!('1.31.1', '0.32.0')).to.equal(false);
      });
    });

    describe('with SquirrelMacEnableDirectContentsWrite enabled', () => {
      updaterIt(
        'should hit the download endpoint when an update is available and update successfully when the zip is provided leaving the parent directory untouched',
        async (ctx) => {
          ctx.setUserDefault('SquirrelMacEnableDirectContentsWrite', true);
          try {
            await ctx.withUpdatableApp(
              {
                nextVersion: '2.0.0',
                startFixture: 'update',
                endFixture: 'update'
              },
              async (appPath, updateZipPath) => {
                const randomID = randomUUID();
                cp.spawnSync('xattr', ['-w', 'spec-id', randomID, appPath]);
                ctx.serveUpdate(updateZipPath);
                const relaunchPromise = ctx.relaunched();
                const launchResult = await ctx.launchApp(appPath, [`http://localhost:${ctx.port}/update-check`]);
                logOnError(launchResult, () => {
                  expect(launchResult).to.have.property('code', 0);
                  expect(launchResult.out).to.include('Update Downloaded');
                  expect(ctx.requests).to.have.lengthOf(2);
                  expect(ctx.requests[0]).to.have.property('url', '/update-check');
                  expect(ctx.requests[1]).to.have.property('url', '/update-file');
                  expect(ctx.requests[0].header('user-agent')).to.include('Electron/');
                  expect(ctx.requests[1].header('user-agent')).to.include('Electron/');
                });

                await relaunchPromise;
                expect(ctx.requests).to.have.lengthOf(3);
                expect(ctx.requests[2].url).to.equal('/update-check/updated/2.0.0');
                expect(ctx.requests[2].header('user-agent')).to.include('Electron/');
                const result = cp.spawnSync('xattr', ['-l', appPath]);
                expect(result.stdout.toString()).to.include(`spec-id: ${randomID}`);
              }
            );
          } finally {
            ctx.setUserDefault('SquirrelMacEnableDirectContentsWrite', null);
          }
        }
      );
    });
  });
});
