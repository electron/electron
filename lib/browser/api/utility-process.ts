import { EventEmitter } from 'events';
import * as path from 'path';
import { MessagePortMain } from '@electron/internal/browser/message-port-main';
const { CreateProcessWrapper } = process._linkedBinding('electron_browser_utility_process');

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

function convertToValidSignal (signal: string | number | undefined): number {
  if (typeof signal === 'number' && getSignalsToNamesMapping()[signal]) { return signal; }

  const { signals } = require('os').constants;

  if (typeof signal === 'string') {
    const signalName = signals[signal.toUpperCase()];
    if (signalName) return signalName;
  }

  return signals.SIGTERM;
}

export default class UtilityProcess extends EventEmitter {
  _handle: any
  constructor (modulePath: string, args: string[] = [], options: Electron.UtilityProcessConstructorOptions) {
    super();
    let relativeEntryPath = null;

    if (!modulePath) {
      throw new Error('Missing UtilityProcess entry script.');
    }

    if (process.resourcesPath) {
      relativeEntryPath = path.relative(process.resourcesPath, modulePath);
    }

    if (!relativeEntryPath) {
      throw new Error('Cannot load entry script from outisde the application.');
    }

    if (args == null) {
      args = [];
    } else if (typeof args === 'object' && !Array.isArray(args)) {
      options = args;
      args = [];
    }

    options = { ...options };

    if (options.execArgv == null) {
      options.execArgv = [];
    } else if (!Array.isArray(options.execArgv)) {
      throw new Error('execArgv must be an array of strings.');
    }

    if (options.serviceName != null) {
      if (typeof options.serviceName !== 'string') {
        throw new Error('displayName must be a string.');
      }
    }

    this._handle = CreateProcessWrapper({ modulePath, args, ...options });
    this._handle.emit = (channel: string, ...args: any[]) => {
      if (channel === 'exit') {
        this.emit('exit', ...args);
        this._handle = null;
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

  postMessage (...args: any[]) {
    if (this._handle === null) {
      return;
    }
    if (Array.isArray(args[2])) {
      args[2] = args[2].map((o: any) => o instanceof MessagePortMain ? o._internalPort : o);
    }
    return this._handle.postMessage(...args);
  }

  kill (signal: string | number) : boolean {
    if (this._handle === null) {
      return false;
    }
    const sig = signal === 0 ? signal
      : convertToValidSignal(signal);
    const err = this._handle.kill(sig);
    if (err === 0) return true;
    return false;
  }
}
