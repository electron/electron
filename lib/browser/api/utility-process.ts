import { EventEmitter } from 'events';
import { Readable } from 'stream';
import * as path from 'path';
import { app } from 'electron/main';
import { MessagePortMain } from '@electron/internal/browser/message-port-main';
const { createProcessWrapper } = process._linkedBinding('electron_browser_utility_process');

// Signal validation is based on
// https://github.com/nodejs/node/blob/main/lib/child_process.js#L938-L946
let signalsToNamesMapping: Record<number, string>;
function getSignalsToNamesMapping () {
  if (signalsToNamesMapping !== undefined) { return signalsToNamesMapping; }

  const { signals } = require('os').constants;
  signalsToNamesMapping = Object.create(null);
  for (const key in signals) { // eslint-disable-line guard-for-in
    signalsToNamesMapping[signals[key]] = key;
  }

  return signalsToNamesMapping;
}

function convertToValidSignal (signal: string | number): number {
  if (typeof signal === 'number' && getSignalsToNamesMapping()[signal]) { return signal; }

  const { signals } = require('os').constants;

  if (typeof signal === 'string') {
    const signalName = signals[signal.toUpperCase()];
    if (signalName) return signalName;
  }

  return signals.SIGTERM;
}

function sanitizeKillSignal (signal: string | number): number {
  if (typeof signal === 'string' || typeof signal === 'number') {
    return convertToValidSignal(signal);
  } else {
    throw new TypeError('Kill signal must be a string or number');
  }
}

class IOReadable extends Readable {
  _shouldPush: boolean = false;
  _data: (Buffer | null)[] = [];
  _resume: (() => void) | null = null;

  _storeInternalData (chunk: Buffer | null, resume: (() => void) | null) {
    this._resume = resume;
    this._data.push(chunk);
    this._pushInternalData();
  }

  _pushInternalData () {
    while (this._shouldPush && this._data.length > 0) {
      const chunk = this._data.shift();
      this._shouldPush = this.push(chunk);
    }
    if (this._shouldPush && this._resume) {
      const resume = this._resume;
      this._resume = null;
      resume();
    }
  }

  _read () {
    this._shouldPush = true;
    this._pushInternalData();
  }
}

export default class UtilityProcess extends EventEmitter {
  _handle: any;
  _stdout: IOReadable | null | undefined = new IOReadable();
  _stderr: IOReadable | null | undefined = new IOReadable();
  constructor (modulePath: string, args?: string[], options?: Electron.UtilityProcessConstructorOptions) {
    super();
    let relativeEntryPath = null;

    if (!modulePath) {
      throw new Error('Missing UtilityProcess entry script.');
    }

    if (process.resourcesPath) {
      relativeEntryPath = path.relative(process.resourcesPath, modulePath);
    }

    if (app.isPackaged && relativeEntryPath && relativeEntryPath.startsWith('..')) {
      throw new Error('Cannot load entry script from outisde the application.');
    }

    if (args == null) {
      args = [];
    } else if (typeof args === 'object' && !Array.isArray(args)) {
      options = args;
      args = [];
    }

    if (options == null) {
      options = Object.create({});
    } else {
      options = { ...options };
    }

    if (options!.execArgv == null) {
      options!.execArgv = process.execArgv;
    } else if (!Array.isArray(options!.execArgv)) {
      throw new Error('execArgv must be an array of strings.');
    }

    if (options!.env == null) {
      options!.env = { ...process.env };
    }

    if (options!.serviceName != null) {
      if (typeof options!.serviceName !== 'string') {
        throw new Error('serviceName must be a string.');
      }
    }

    if (typeof options!.stdio === 'string') {
      const stdio : Array<'pipe' | 'ignore' | 'inherit'> = [];
      switch (options!.stdio) {
        case 'inherit':
        case 'ignore':
          this._stdout = null;
          this._stderr = null;
          // falls through
        case 'pipe':
          stdio.push('ignore', options!.stdio, options!.stdio);
          break;
        default:
          throw new Error('stdio must be of the following values: inherit, pipe, ignore');
      }
      options!.stdio = stdio;
    } else if (Array.isArray(options!.stdio)) {
      if (options!.stdio.length >= 3) {
        if (options!.stdio[0] !== 'ignore') {
          throw new Error('stdin value other than ignore is not supported.');
        }
        if (options!.stdio[1] === 'ignore' || options!.stdio[1] === 'inherit') {
          this._stdout = null;
        } else if (options!.stdio[1] !== 'pipe') {
          throw new Error('stdout configuration must be of the following values: inherit, pipe, ignore');
        }
        if (options!.stdio[2] === 'ignore' || options!.stdio[2] === 'inherit') {
          this._stderr = null;
        } else if (options!.stdio[2] !== 'pipe') {
          throw new Error('stderr configuration must be of the following values: inherit, pipe, ignore');
        }
      } else {
        throw new Error('configuration missing for stdin, stdout or stderr.');
      }
    }

    this._handle = createProcessWrapper({ modulePath, args, ...options });
    this._handle.emit = (channel: string, ...args: any[]) => {
      if (channel === 'exit') {
        this.emit('exit', ...args);
        this._handle = null;
        if (this._stdout) {
          this._stdout.removeAllListeners();
          this._stdout = null;
        }
        if (this._stderr) {
          this._stderr.removeAllListeners();
          this._stderr = null;
        }
      } else if (channel === 'stdout') {
        this._stdout!._storeInternalData(Buffer.from(args[1]), args[2]);
      } else if (channel === 'stderr') {
        this._stderr!._storeInternalData(Buffer.from(args[1]), args[2]);
      } else {
        this.emit(channel, ...args);
      }
    };
  }

  get pid () {
    if (this._handle === null) {
      return undefined;
    }
    return this._handle.pid;
  }

  get stdout () {
    if (this._handle === null) {
      return undefined;
    }
    return this._stdout;
  }

  get stderr () {
    if (this._handle === null) {
      return undefined;
    }
    return this._stderr;
  }

  postMessage (...args: any[]) {
    if (this._handle === null) {
      return;
    }
    if (Array.isArray(args[1])) {
      args[1] = args[1].map((o: any) => o instanceof MessagePortMain ? o._internalPort : o);
    }
    return this._handle.postMessage(...args);
  }

  kill (signal: string | number) : boolean {
    if (this._handle === null) {
      return false;
    }
    signal = signal || 'SIGTERM';
    const sig = signal === 0 ? signal
      : sanitizeKillSignal(signal);
    const err = this._handle.kill(sig);
    if (err === 0) return true;
    return false;
  }
}
