import { app } from 'electron';
import { expect } from 'chai';
import { startRemoteControlApp, ifdescribe } from './lib/spec-helpers';

import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import * as uuid from 'uuid';
import { once } from 'node:events';

function isTestingBindingAvailable () {
  try {
    process._linkedBinding('electron_common_testing');
    return true;
  } catch {
    return false;
  }
}

// This test depends on functions that are only available when DCHECK_IS_ON.
ifdescribe(isTestingBindingAvailable())('logging', () => {
  it('does not log by default', async () => {
    // ELECTRON_ENABLE_LOGGING is turned on in the appveyor config.
    const { ELECTRON_ENABLE_LOGGING: _, ...envWithoutEnableLogging } = process.env;
    const rc = await startRemoteControlApp([], { env: envWithoutEnableLogging });
    const stderrComplete = new Promise<string>(resolve => {
      let stderr = '';
      rc.process.stderr!.on('data', function listener (chunk) {
        stderr += chunk.toString('utf8');
      });
      rc.process.on('close', () => { resolve(stderr); });
    });
    const [hasLoggingSwitch, hasLoggingVar] = await rc.remotely(() => {
      // Make sure we're actually capturing stderr by logging a known value to
      // stderr.
      console.error('SENTINEL');
      process._linkedBinding('electron_common_testing').log(0, 'TEST_LOG');
      setTimeout(() => { process.exit(0); });
      return [require('electron').app.commandLine.hasSwitch('enable-logging'), !!process.env.ELECTRON_ENABLE_LOGGING];
    });
    expect(hasLoggingSwitch).to.be.false();
    expect(hasLoggingVar).to.be.false();
    const stderr = await stderrComplete;
    // stderr should include the sentinel but not the LOG() message.
    expect(stderr).to.match(/SENTINEL/);
    expect(stderr).not.to.match(/TEST_LOG/);
  });

  it('logs to stderr when --enable-logging is passed', async () => {
    const rc = await startRemoteControlApp(['--enable-logging']);
    const stderrComplete = new Promise<string>(resolve => {
      let stderr = '';
      rc.process.stderr!.on('data', function listener (chunk) {
        stderr += chunk.toString('utf8');
      });
      rc.process.on('close', () => { resolve(stderr); });
    });
    rc.remotely(() => {
      process._linkedBinding('electron_common_testing').log(0, 'TEST_LOG');
      setTimeout(() => { require('electron').app.quit(); });
    });
    const stderr = await stderrComplete;
    expect(stderr).to.match(/TEST_LOG/);
  });

  it('logs to stderr when ELECTRON_ENABLE_LOGGING is set', async () => {
    const rc = await startRemoteControlApp([], { env: { ...process.env, ELECTRON_ENABLE_LOGGING: '1' } });
    const stderrComplete = new Promise<string>(resolve => {
      let stderr = '';
      rc.process.stderr!.on('data', function listener (chunk) {
        stderr += chunk.toString('utf8');
      });
      rc.process.on('close', () => { resolve(stderr); });
    });
    rc.remotely(() => {
      process._linkedBinding('electron_common_testing').log(0, 'TEST_LOG');
      setTimeout(() => { require('electron').app.quit(); });
    });
    const stderr = await stderrComplete;
    expect(stderr).to.match(/TEST_LOG/);
  });

  it('logs to a file in the user data dir when --enable-logging=file is passed', async () => {
    const rc = await startRemoteControlApp(['--enable-logging=file']);
    const userDataDir = await rc.remotely(() => {
      const { app } = require('electron');
      process._linkedBinding('electron_common_testing').log(0, 'TEST_LOG');
      setTimeout(() => { app.quit(); });
      return app.getPath('userData');
    });
    await once(rc.process, 'exit');
    const logFilePath = path.join(userDataDir, 'electron_debug.log');
    const stat = await fs.stat(logFilePath);
    expect(stat.isFile()).to.be.true();
    const contents = await fs.readFile(logFilePath, 'utf8');
    expect(contents).to.match(/TEST_LOG/);
  });

  it('logs to a file in the user data dir when ELECTRON_ENABLE_LOGGING=file is set', async () => {
    const rc = await startRemoteControlApp([], { env: { ...process.env, ELECTRON_ENABLE_LOGGING: 'file' } });
    const userDataDir = await rc.remotely(() => {
      const { app } = require('electron');
      process._linkedBinding('electron_common_testing').log(0, 'TEST_LOG');
      setTimeout(() => { app.quit(); });
      return app.getPath('userData');
    });
    await once(rc.process, 'exit');
    const logFilePath = path.join(userDataDir, 'electron_debug.log');
    const stat = await fs.stat(logFilePath);
    expect(stat.isFile()).to.be.true();
    const contents = await fs.readFile(logFilePath, 'utf8');
    expect(contents).to.match(/TEST_LOG/);
  });

  it('logs to the given file when --log-file is passed', async () => {
    const logFilePath = path.join(app.getPath('temp'), 'test-log-file-' + uuid.v4());
    const rc = await startRemoteControlApp(['--enable-logging', '--log-file=' + logFilePath]);
    rc.remotely(() => {
      process._linkedBinding('electron_common_testing').log(0, 'TEST_LOG');
      setTimeout(() => { require('electron').app.quit(); });
    });
    await once(rc.process, 'exit');
    const stat = await fs.stat(logFilePath);
    expect(stat.isFile()).to.be.true();
    const contents = await fs.readFile(logFilePath, 'utf8');
    expect(contents).to.match(/TEST_LOG/);
  });

  it('logs to the given file when ELECTRON_LOG_FILE is set', async () => {
    const logFilePath = path.join(app.getPath('temp'), 'test-log-file-' + uuid.v4());
    const rc = await startRemoteControlApp([], { env: { ...process.env, ELECTRON_ENABLE_LOGGING: '1', ELECTRON_LOG_FILE: logFilePath } });
    rc.remotely(() => {
      process._linkedBinding('electron_common_testing').log(0, 'TEST_LOG');
      setTimeout(() => { require('electron').app.quit(); });
    });
    await once(rc.process, 'exit');
    const stat = await fs.stat(logFilePath);
    expect(stat.isFile()).to.be.true();
    const contents = await fs.readFile(logFilePath, 'utf8');
    expect(contents).to.match(/TEST_LOG/);
  });

  it('does not lose early log messages when logging to a given file with --log-file', async () => {
    const logFilePath = path.join(app.getPath('temp'), 'test-log-file-' + uuid.v4());
    const rc = await startRemoteControlApp(['--enable-logging', '--log-file=' + logFilePath, '--boot-eval=process._linkedBinding(\'electron_common_testing\').log(0, \'EARLY_LOG\')']);
    rc.remotely(() => {
      process._linkedBinding('electron_common_testing').log(0, 'LATER_LOG');
      setTimeout(() => { require('electron').app.quit(); });
    });
    await once(rc.process, 'exit');
    const stat = await fs.stat(logFilePath);
    expect(stat.isFile()).to.be.true();
    const contents = await fs.readFile(logFilePath, 'utf8');
    expect(contents).to.match(/EARLY_LOG/);
    expect(contents).to.match(/LATER_LOG/);
  });

  it('enables logging when switch is appended during first tick', async () => {
    const rc = await startRemoteControlApp(['--boot-eval=require(\'electron\').app.commandLine.appendSwitch(\'--enable-logging\')']);
    const stderrComplete = new Promise<string>(resolve => {
      let stderr = '';
      rc.process.stderr!.on('data', function listener (chunk) {
        stderr += chunk.toString('utf8');
      });
      rc.process.on('close', () => { resolve(stderr); });
    });
    rc.remotely(() => {
      process._linkedBinding('electron_common_testing').log(0, 'TEST_LOG');
      setTimeout(() => { require('electron').app.quit(); });
    });
    const stderr = await stderrComplete;
    expect(stderr).to.match(/TEST_LOG/);
  });

  it('respects --log-level', async () => {
    const rc = await startRemoteControlApp(['--enable-logging', '--log-level=1']);
    const stderrComplete = new Promise<string>(resolve => {
      let stderr = '';
      rc.process.stderr!.on('data', function listener (chunk) {
        stderr += chunk.toString('utf8');
      });
      rc.process.on('close', () => { resolve(stderr); });
    });
    rc.remotely(() => {
      process._linkedBinding('electron_common_testing').log(0, 'TEST_INFO_LOG');
      process._linkedBinding('electron_common_testing').log(1, 'TEST_WARNING_LOG');
      setTimeout(() => { require('electron').app.quit(); });
    });
    const stderr = await stderrComplete;
    expect(stderr).to.match(/TEST_WARNING_LOG/);
    expect(stderr).not.to.match(/TEST_INFO_LOG/);
  });
});
