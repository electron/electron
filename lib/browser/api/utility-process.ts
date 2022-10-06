import { EventEmitter } from 'events';
import { PassThrough } from 'stream';
import { createReadStream } from 'fs';
import { MessagePortMain } from '@electron/internal/browser/message-port-main';
const { _fork } = process._linkedBinding('electron_browser_utility_process');

class ForkUtilityProcess extends EventEmitter {
  #handle: ElectronInternal.UtilityProcessWrapper | null;
  #stdout: any = null;
  #stderr: any = null;
  constructor (modulePath: string, args?: string[], options?: Electron.ForkOptions) {
    super();

    if (!modulePath) {
      throw new Error('Missing UtilityProcess entry script.');
    }

    if (args == null) {
      args = [];
    } else if (typeof args === 'object' && !Array.isArray(args)) {
      options = args;
      args = [];
    }

    if (options == null) {
      options = {};
    } else {
      options = { ...options };
    }

    if (!options) {
      throw new Error('Options cannot be undefined.');
    }

    if (options.execArgv == null) {
      options.execArgv = process.execArgv;
    } else if (!Array.isArray(options.execArgv)) {
      throw new Error('execArgv must be an array of strings.');
    }

    if (options.serviceName != null) {
      if (typeof options.serviceName !== 'string') {
        throw new Error('serviceName must be a string.');
      }
    }

    if (options.cwd != null) {
      if (typeof options.cwd !== 'string') {
        throw new Error('cwd path must be a string.');
      }
    }

    if (typeof options.stdio === 'string') {
      const stdio : Array<'pipe' | 'ignore' | 'inherit'> = [];
      switch (options.stdio) {
        case 'inherit':
        case 'ignore':
          stdio.push('ignore', options.stdio, options.stdio);
          break;
        case 'pipe':
          this.#stderr = new PassThrough();
          this.#stdout = new PassThrough();
          stdio.push('ignore', options.stdio, options.stdio);
          break;
        default:
          throw new Error('stdio must be of the following values: inherit, pipe, ignore');
      }
      options.stdio = stdio;
    } else if (Array.isArray(options.stdio)) {
      if (options.stdio.length >= 3) {
        if (options.stdio[0] !== 'ignore') {
          throw new Error('stdin value other than ignore is not supported.');
        }

        if (options.stdio[1] === 'pipe') {
          this.#stdout = new PassThrough();
        } else if (options.stdio[1] !== 'ignore' && options.stdio[1] !== 'inherit') {
          throw new Error('stdout configuration must be of the following values: inherit, pipe, ignore');
        }

        if (options.stdio[2] === 'pipe') {
          this.#stderr = new PassThrough();
        } else if (options.stdio[2] !== 'ignore' && options.stdio[2] !== 'inherit') {
          throw new Error('stderr configuration must be of the following values: inherit, pipe, ignore');
        }
      } else {
        throw new Error('configuration missing for stdin, stdout or stderr.');
      }
    }

    this.#handle = _fork({ options, modulePath, args });
    this.#handle!.emit = (channel: string | symbol, ...args: any[]) => {
      if (channel === 'exit') {
        try {
          this.emit('exit', ...args);
        } finally {
          this.#handle = null;
          if (this.#stdout) {
            this.#stdout.removeAllListeners();
            this.#stdout = null;
          }
          if (this.#stderr) {
            this.#stderr.removeAllListeners();
            this.#stderr = null;
          }
        }
        return false;
      } else if (channel === 'stdout' && this.#stdout) {
        createReadStream('', { fd: args[0] }).pipe(this.#stdout);
        return false;
      } else if (channel === 'stderr' && this.stderr) {
        createReadStream('', { fd: args[0] }).pipe(this.#stderr);
        return false;
      } else {
        return this.emit(channel, ...args);
      }
    };
  }

  get pid () {
    return this.#handle?.pid;
  }

  get stdout () {
    return this.#stdout;
  }

  get stderr () {
    return this.#stderr;
  }

  postMessage (message: any, transfer?: MessagePortMain[]) {
    if (Array.isArray(transfer)) {
      transfer = transfer.map((o: any) => o instanceof MessagePortMain ? o._internalPort : o);
      return this.#handle?.postMessage(message, transfer);
    }
    return this.#handle?.postMessage(message);
  }

  kill () : boolean {
    if (this.#handle === null) {
      return false;
    }
    return this.#handle.kill();
  }
}

export function fork (modulePath: string, args?: string[], options?: Electron.ForkOptions) {
  return new ForkUtilityProcess(modulePath, args, options);
}
