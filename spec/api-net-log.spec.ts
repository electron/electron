import { session, net } from 'electron/main';

import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it } from 'vitest';

import * as ChildProcess from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import { createRequire } from 'node:module';
import * as os from 'node:os';
import * as path from 'node:path';

import { ifit, listen } from './lib/spec-helpers.ts';

import type { Socket } from 'node:net';

const require = createRequire(import.meta.url);

const appPath = path.join(import.meta.dirname, 'fixtures', 'api', 'net-log');
const dumpFile = path.join(os.tmpdir(), 'net_log.json');
const dumpFileDynamic = path.join(os.tmpdir(), 'net_log_dynamic.json');

const testNetLog = () => session.fromPartition('net-log').netLog;

describe('netLog module', () => {
  let server: http.Server;
  let serverUrl: string;
  const connections: Set<Socket> = new Set();

  beforeAll(async () => {
    server = http.createServer();
    server.on('connection', (connection) => {
      connections.add(connection);
      connection.once('close', () => {
        connections.delete(connection);
      });
    });
    server.on('request', (request, response) => {
      response.end();
    });
    serverUrl = (await listen(server)).url;
  });

  afterAll(
    () =>
      new Promise<void>((resolve) => {
        for (const connection of connections) {
          connection.destroy();
        }
        server.close(() => {
          server = null as any;
          resolve();
        });
      })
  );

  beforeEach(() => {
    expect(testNetLog().currentlyLogging, 'currently logging').toBe(false);
  });
  afterEach(async () => {
    // Stop a log that a failed test left running. Otherwise this hook and
    // beforeEach would fail every later test, including the retry.
    if (testNetLog().currentlyLogging) {
      await testNetLog().stopLogging();
    }
    try {
      if (fs.existsSync(dumpFile)) {
        fs.unlinkSync(dumpFile);
      }
      if (fs.existsSync(dumpFileDynamic)) {
        fs.unlinkSync(dumpFileDynamic);
      }
    } catch {
      // Ignore error
    }
  });

  it('should begin and end logging to file when .startLogging() and .stopLogging() is called', async () => {
    await testNetLog().startLogging(dumpFileDynamic);

    expect(testNetLog().currentlyLogging, 'currently logging').toBe(true);

    await testNetLog().stopLogging();

    expect(fs.existsSync(dumpFileDynamic), 'currently logging').toBe(true);
  });

  it('should throw an error when .stopLogging() is called without calling .startLogging()', async () => {
    await expect(testNetLog().stopLogging()).rejects.toThrow('No net log in progress');
  });

  it('should throw an error when .startLogging() is called with an invalid argument', () => {
    expect(() => testNetLog().startLogging('')).toThrow();
    expect(() => testNetLog().startLogging(null as any)).toThrow();
    expect(() => testNetLog().startLogging([] as any)).toThrow();
    expect(() => testNetLog().startLogging('aoeu', { captureMode: 'aoeu' as any })).toThrow();
    expect(() => testNetLog().startLogging('aoeu', { maxFileSize: null as any })).toThrow();
  });

  it('should include cookies when requested', async () => {
    await testNetLog().startLogging(dumpFileDynamic, { captureMode: 'includeSensitive' });
    const unique = require('uuid').v4();
    await new Promise<void>((resolve) => {
      const req = net.request(serverUrl);
      req.setHeader('Cookie', `foo=${unique}`);
      req.on('response', (response) => {
        response.on('data', () => {}); // https://github.com/electron/electron/issues/19214
        response.on('end', () => resolve());
      });
      req.end();
    });
    await testNetLog().stopLogging();
    expect(fs.existsSync(dumpFileDynamic), 'dump file exists').toBe(true);
    const dump = fs.readFileSync(dumpFileDynamic, 'utf8');
    expect(dump).toContain(`foo=${unique}`);
  });

  it('should include socket bytes when requested', async () => {
    await testNetLog().startLogging(dumpFileDynamic, { captureMode: 'everything' });
    const unique = require('uuid').v4();
    await new Promise<void>((resolve) => {
      const req = net.request({ method: 'POST', url: serverUrl });
      req.on('response', (response) => {
        response.on('data', () => {}); // https://github.com/electron/electron/issues/19214
        response.on('end', () => resolve());
      });
      req.end(Buffer.from(unique));
    });
    await testNetLog().stopLogging();
    expect(fs.existsSync(dumpFileDynamic), 'dump file exists').toBe(true);
    const dump = fs.readFileSync(dumpFileDynamic, 'utf8');
    expect(
      JSON.parse(dump).events.some(
        (x: any) => x.params && x.params.bytes && Buffer.from(x.params.bytes, 'base64').includes(unique)
      ),
      'uuid present in dump'
    ).toBe(true);
  });

  ifit(process.platform !== 'linux')(
    'should begin and end logging automatically when --log-net-log is passed',
    async () => {
      const appProcess = ChildProcess.spawn(process.execPath, [appPath], {
        env: {
          TEST_REQUEST_URL: serverUrl,
          TEST_DUMP_FILE: dumpFile
        }
      });

      await once(appProcess, 'exit');
      expect(fs.existsSync(dumpFile), 'dump file exists').toBe(true);
    }
  );

  ifit(process.platform !== 'linux')(
    'should begin and end logging automatically when --log-net-log is passed, and behave correctly when .startLogging() and .stopLogging() is called',
    async () => {
      const appProcess = ChildProcess.spawn(process.execPath, [appPath], {
        env: {
          TEST_REQUEST_URL: serverUrl,
          TEST_DUMP_FILE: dumpFile,
          TEST_DUMP_FILE_DYNAMIC: dumpFileDynamic,
          TEST_MANUAL_STOP: 'true'
        }
      });

      await once(appProcess, 'exit');
      expect(fs.existsSync(dumpFile), 'dump file exists').toBe(true);
      expect(fs.existsSync(dumpFileDynamic), 'dynamic dump file exists').toBe(true);
    }
  );

  ifit(process.platform !== 'linux')(
    'should end logging automatically when only .startLogging() is called',
    async () => {
      const appProcess = ChildProcess.spawn(process.execPath, [appPath], {
        env: {
          TEST_REQUEST_URL: serverUrl,
          TEST_DUMP_FILE_DYNAMIC: dumpFileDynamic
        }
      });

      await once(appProcess, 'exit');
      expect(fs.existsSync(dumpFileDynamic), 'dynamic dump file exists').toBe(true);
    }
  );
});
