import * as childProcess from 'child_process';
import * as path from 'path';
import * as http from 'http';
import * as https from 'https';
import * as net from 'net';
import * as v8 from 'v8';
import * as url from 'url';
import { BrowserWindow } from 'electron/main';
import { AssertionError } from 'chai';

// const addOnly = <T>(fn: Function): T => {
//   const wrapped = (...args: any[]) => {
//     return fn(...args);
//   };
//   (wrapped as any).only = wrapped;
//   (wrapped as any).skip = wrapped;
//   return wrapped as any;
// };

// export const ifit = (condition: boolean) => (condition ? it : addOnly<TestFunction>(it.skip));
// export const ifdescribe = (condition: boolean) => (condition ? describe : addOnly<SuiteFunction>(describe.skip));

type CleanupFunction = (() => void) | (() => Promise<void>)
const cleanupFunctions: CleanupFunction[] = [];
export async function runCleanupFunctions () {
  for (const cleanup of cleanupFunctions) {
    const r = cleanup();
    if (r instanceof Promise) { await r; }
  }
  cleanupFunctions.length = 0;
}

export function defer (f: CleanupFunction) {
  cleanupFunctions.unshift(f);
}

class RemoteControlApp {
  process: childProcess.ChildProcess;
  port: number;

  constructor (proc: childProcess.ChildProcess, port: number) {
    this.process = proc;
    this.port = port;
  }

  remoteEval = (js: string): Promise<any> => {
    return new Promise((resolve, reject) => {
      const req = http.request({
        host: '127.0.0.1',
        port: this.port,
        method: 'POST'
      }, res => {
        const chunks = [] as Buffer[];
        res.on('data', chunk => { chunks.push(chunk); });
        res.on('end', () => {
          const ret = v8.deserialize(Buffer.concat(chunks));
          if (Object.prototype.hasOwnProperty.call(ret, 'error')) {
            reject(new Error(`remote error: ${ret.error}\n\nTriggered at:`));
          } else {
            resolve(ret.result);
          }
        });
      });
      req.write(js);
      req.end();
    });
  }

  remotely = (script: Function, ...args: any[]): Promise<any> => {
    return this.remoteEval(`(${script})(...${JSON.stringify(args)})`);
  }
}

export async function startRemoteControlApp (extraArgs: string[] = [], options?: childProcess.SpawnOptionsWithoutStdio) {
  const appPath = path.join(__dirname, '..', 'fixtures', 'apps', 'remote-control');
  const appProcess = childProcess.spawn(process.execPath, [appPath, ...extraArgs], options);
  appProcess.stderr.on('data', d => {
    process.stderr.write(d);
  });
  const port = await new Promise<number>(resolve => {
    appProcess.stdout.on('data', d => {
      const m = /Listening: (\d+)/.exec(d.toString());
      if (m && m[1] != null) {
        resolve(Number(m[1]));
      }
    });
  });
  defer(() => { appProcess.kill('SIGINT'); });
  return new RemoteControlApp(appProcess, port);
}

async function makeRemoteContext (opts?: any) {
  const { webPreferences, setup, url = 'about:blank', ...rest } = opts ?? {};
  const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false, ...webPreferences }, ...rest });
  await w.loadURL(url.toString());
  if (setup) await w.webContents.executeJavaScript(setup);
  return w;
}

const remoteContext: BrowserWindow[] = [];
export async function getRemoteContext () {
  if (remoteContext.length) { return remoteContext[0]; }
  const w = await makeRemoteContext();
  defer(() => w.close());
  return w;
}

export function useRemoteContext (opts?: any) {
  before(async () => {
    remoteContext.unshift(await makeRemoteContext(opts));
  });
  after(() => {
    const w = remoteContext.shift();
    w!.close();
  });
}

export async function itremote (name: string, fn: Function, args?: any[]) {
  it(name, async () => {
    const w = await getRemoteContext();
    const { ok, message } = await w.webContents.executeJavaScript(`(async () => {
      try {
        const chai_1 = require('chai')
        const promises_1 = require('timers/promises')
        chai_1.use(require('chai-as-promised'))
        chai_1.use(require('dirty-chai'))
        await (${fn})(...${JSON.stringify(args ?? [])})
        return {ok: true};
      } catch (e) {
        return {ok: false, message: e.message}
      }
    })()`);
    if (!ok) { throw new AssertionError(message); }
  });
}

export async function listen (server: http.Server | https.Server) {
  const hostname = '127.0.0.1';
  await new Promise<void>(resolve => server.listen(0, hostname, () => resolve()));
  const { port } = server.address() as net.AddressInfo;
  const protocol = (server instanceof https.Server) ? 'https' : 'http';
  return { port, url: url.format({ protocol, hostname, port }) };
}
