import { EventEmitter } from 'events';
import { Readable } from 'stream';
import * as path from 'path';
import { app } from 'electron/main';
import { MessagePortMain } from '@electron/internal/browser/message-port-main';
const { createProcessWrapper } = process._linkedBinding('electron_browser_utility_process');

class IOReadable extends Readable {
  #shouldPush: boolean = false;
  #data: (Buffer | null)[] = [];
  #resume: (() => void) | null = null;

  storeInternalData (chunk: Buffer | null, resume: (() => void) | null) {
    this.#resume = resume;
    this.#data.push(chunk);
    this.#pushInternalData();
  }

  #pushInternalData () {
    while (this.#shouldPush && this.#data.length > 0) {
      const chunk = this.#data.shift();
      this.#shouldPush = this.push(chunk);
    }
    if (this.#shouldPush && this.#resume) {
      const resume = this.#resume;
      this.#resume = null;
      resume();
    }
  }

  _read () {
    this.#shouldPush = true;
    this.#pushInternalData();
  }
}

export default class UtilityProcess extends EventEmitter {
  #handle: ElectronInternal.UtilityProcessWrapper | null;
  #stdout: IOReadable | null | undefined = new IOReadable();
  #stderr: IOReadable | null | undefined = new IOReadable();
  constructor (modulePath: string, args?: string[], options?: Electron.UtilityProcessConstructorOptions) {
    super();

    if (!modulePath) {
      throw new Error('Missing UtilityProcess entry script.');
    }

    const relativeEntryPath = path.relative(process.resourcesPath, modulePath);

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

    if (typeof options.stdio === 'string') {
      const stdio : Array<'pipe' | 'ignore' | 'inherit'> = [];
      switch (options.stdio) {
        case 'inherit':
        case 'ignore':
          this.#stdout = null;
          this.#stderr = null;
          // falls through
        case 'pipe':
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
        if (options.stdio[1] === 'ignore' || options.stdio[1] === 'inherit') {
          this.#stdout = null;
        } else if (options.stdio[1] !== 'pipe') {
          throw new Error('stdout configuration must be of the following values: inherit, pipe, ignore');
        }
        if (options.stdio[2] === 'ignore' || options.stdio[2] === 'inherit') {
          this.#stderr = null;
        } else if (options.stdio[2] !== 'pipe') {
          throw new Error('stderr configuration must be of the following values: inherit, pipe, ignore');
        }
      } else {
        throw new Error('configuration missing for stdin, stdout or stderr.');
      }
    }

    this.#handle = createProcessWrapper({ options, modulePath, args });
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
        this.#stdout.storeInternalData(Buffer.from(args[1]), args[2]);
        return false;
      } else if (channel === 'stderr' && this.#stderr) {
        this.#stderr.storeInternalData(Buffer.from(args[1]), args[2]);
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
    if (this.#handle === null) {
      return undefined;
    }
    return this.#stdout;
  }

  get stderr () {
    if (this.#handle === null) {
      return undefined;
    }
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
