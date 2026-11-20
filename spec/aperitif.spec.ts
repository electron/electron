import { expect, it } from 'vitest';

import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { ifdescribe, spawnAndWait } from './lib/spec-helpers.ts';

// The bundled library; component builds load it from the output directory
// instead, along with other component libraries whose initializers would run
// first, so they are not checked.
const bundledAperitif = path.resolve(
  path.dirname(process.execPath),
  '..',
  'Frameworks',
  'Electron Framework.framework',
  'Libraries',
  'libelectron_aperitif.dylib'
);

// On macOS 13, DYLD_PRINT_INITIALIZERS crashes dyld itself; fixed in macOS 14
// (Darwin 23). Chromium's AperitifTest skips the check there too.
const canCheckInitializerOrder =
  process.platform === 'darwin' && Number(os.release().split('.')[0]) >= 23 && fs.existsSync(bundledAperitif);

const kInitializerLine = /^dyld\[(\d+)\]: running initializer 0x[0-9a-f]+ in (.*)$/;

// Only foundational OS libraries may run initializers before
// libelectron_aperitif. Do not add more without a security review: code that
// runs here runs before the sandbox is engaged.
const isFoundational = (library: string) =>
  library === '/usr/lib/libSystem.B.dylib' || library.startsWith('/usr/lib/system/');

interface InitializerOrder {
  sawInitializers: boolean;
  sawAperitif: boolean;
  // Initializers outside the allowlist that ran before libelectron_aperitif's.
  disallowed: string[];
}

function parseInitializerOrder(stderr: string): Map<number, InitializerOrder> {
  const byPid = new Map<number, InitializerOrder>();
  for (const line of stderr.split('\n')) {
    const match = kInitializerLine.exec(line.trim());
    if (!match) continue;
    const pid = Number(match[1]);
    const library = match[2];
    let order = byPid.get(pid);
    if (!order) {
      order = { sawInitializers: true, sawAperitif: false, disallowed: [] };
      byPid.set(pid, order);
    }
    if (order.sawAperitif) continue;
    if (library.endsWith('/libelectron_aperitif.dylib')) {
      order.sawAperitif = true;
    } else if (!isFoundational(library)) {
      order.disallowed.push(library);
    }
  }
  return byPid;
}

ifdescribe(canCheckInitializerOrder)('libelectron_aperitif', () => {
  it('initializes before Electron Framework and system libraries in the browser and a sandboxed renderer', async () => {
    const env: NodeJS.ProcessEnv = { ...process.env, DYLD_PRINT_INITIALIZERS: '1' };
    delete env.ELECTRON_RUN_AS_NODE;
    const appPath = path.resolve(import.meta.dirname, 'fixtures', 'apps', 'aperitif-initializer-order', 'main.js');
    const { code, stdout, stderr } = await spawnAndWait(process.execPath, [appPath], { env, timeout: 60_000 });
    expect(code, stderr.slice(-4000)).toBe(0);

    const resultLine = stdout
      .split('\n')
      .reverse()
      .find((line) => line.startsWith('{'));
    expect(resultLine, `no result from the fixture app:\n${stdout}`).toBeDefined();
    const { browser, renderer } = JSON.parse(resultLine!) as { browser: number; renderer: number };

    const orders = parseInitializerOrder(stderr);
    // Distinguish a change in dyld's output format from a real regression.
    expect(
      orders.size,
      `no DYLD_PRINT_INITIALIZERS output at all; did its format change?\n${stderr.slice(-4000)}`
    ).toBeGreaterThan(0);

    for (const [pid, type] of [
      [browser, 'browser'],
      [renderer, 'renderer']
    ] as const) {
      const order = orders.get(pid);
      expect(order?.sawInitializers, `${type} process ${pid} printed no initializers`).toBe(true);
      expect(order!.disallowed, `initializers that ran before libelectron_aperitif in the ${type} process`).toEqual([]);
      expect(order!.sawAperitif, `libelectron_aperitif did not initialize in the ${type} process`).toBe(true);
    }
  });
});
