import * as childProcess from 'child_process';
import * as path from 'path';
import * as http from 'http';
import * as v8 from 'v8';

export const ifit = (condition: boolean) => (condition ? it : it.skip);
export const ifdescribe = (condition: boolean) => (condition ? describe : describe.skip);

export const delay = (time: number = 0) => new Promise(resolve => setTimeout(resolve, time));

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

export async function startRemoteControlApp () {
  const appPath = path.join(__dirname, 'fixtures', 'apps', 'remote-control');
  const appProcess = childProcess.spawn(process.execPath, [appPath]);
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
