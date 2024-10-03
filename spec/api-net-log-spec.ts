import { session, net } from 'electron/main';

import { expect } from 'chai';

import * as ChildProcess from 'node:child_process';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as http from 'node:http';
import { Socket } from 'node:net';
import * as os from 'node:os';
import * as path from 'node:path';

import { ifit, listen } from './lib/spec-helpers';

const appPath = path.join(__dirname, 'fixtures', 'api', 'net-log');
const dumpFile = path.join(os.tmpdir(), 'net_log.json');
const dumpFileDynamic = path.join(os.tmpdir(), 'net_log_dynamic.json');

const testNetLog = () => session.fromPartition('net-log').netLog;

describe('netLog module', () => {
  let server: http.Server;
  let serverUrl: string;
  const connections: Set<Socket> = new Set();

  before(async () => {
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

  after(done => {
    for (const connection of connections) {
      connection.destroy();
    }
    server.close(() => {
      server = null as any;
      done();
    });
  });

  beforeEach(() => {
    expect(testNetLog().currentlyLogging).to.be.false('currently logging');
  });
  afterEach(() => {
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
    expect(testNetLog().currentlyLogging).to.be.false('currently logging');
  });

  it('should begin and end logging to file when .startLogging() and .stopLogging() is called', async () => {
    await testNetLog().startLogging(dumpFileDynamic);

    expect(testNetLog().currentlyLogging).to.be.true('currently logging');

    await testNetLog().stopLogging();

    expect(fs.existsSync(dumpFileDynamic)).to.be.true('currently logging');
  });

  it('should throw an error when .stopLogging() is called without calling .startLogging()', async () => {
    await expect(testNetLog().stopLogging()).to.be.rejectedWith('No net log in progress');
  });

  it('should throw an error when .startLogging() is called with an invalid argument', () => {
    expect(() => testNetLog().startLogging('')).to.throw();
    expect(() => testNetLog().startLogging(null as any)).to.throw();
    expect(() => testNetLog().startLogging([] as any)).to.throw();
    expect(() => testNetLog().startLogging('aoeu', { captureMode: 'aoeu' as any })).to.throw();
    expect(() => testNetLog().startLogging('aoeu', { maxFileSize: null as any })).to.throw();
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
    expect(fs.existsSync(dumpFileDynamic)).to.be.true('dump file exists');
    const dump = fs.readFileSync(dumpFileDynamic, 'utf8');
    expect(dump).to.contain(`foo=${unique}`);
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
    expect(fs.existsSync(dumpFileDynamic)).to.be.true('dump file exists');
    const dump = fs.readFileSync(dumpFileDynamic, 'utf8');
    expect(JSON.parse(dump).events.some((x: any) => x.params && x.params.bytes && Buffer.from(x.params.bytes, 'base64').includes(unique))).to.be.true('uuid present in dump');
  });

  ifit(process.platform !== 'linux')('should begin and end logging automatically when --log-net-log is passed', async () => {
    const appProcess = ChildProcess.spawn(process.execPath,
      [appPath], {
        env: {
          TEST_REQUEST_URL: serverUrl,
          TEST_DUMP_FILE: dumpFile
        }
      });

    await once(appProcess, 'exit');
    expect(fs.existsSync(dumpFile)).to.be.true('dump file exists');
  });

  ifit(process.platform !== 'linux')('should begin and end logging automatically when --log-net-log is passed, and behave correctly when .startLogging() and .stopLogging() is called', async () => {
    const appProcess = ChildProcess.spawn(process.execPath,
      [appPath], {
        env: {
          TEST_REQUEST_URL: serverUrl,
          TEST_DUMP_FILE: dumpFile,
          TEST_DUMP_FILE_DYNAMIC: dumpFileDynamic,
          TEST_MANUAL_STOP: 'true'
        }
      });

    await once(appProcess, 'exit');
    expect(fs.existsSync(dumpFile)).to.be.true('dump file exists');
    expect(fs.existsSync(dumpFileDynamic)).to.be.true('dynamic dump file exists');
  });

  ifit(process.platform !== 'linux')('should end logging automatically when only .startLogging() is called', async () => {
    const appProcess = ChildProcess.spawn(process.execPath,
      [appPath], {
        env: {
          TEST_REQUEST_URL: serverUrl,
          TEST_DUMP_FILE_DYNAMIC: dumpFileDynamic
        }
      });

    await once(appProcess, 'exit');
    expect(fs.existsSync(dumpFileDynamic)).to.be.true('dynamic dump file exists');
  });
});
