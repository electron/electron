import { EventEmitter } from 'events';
import { ClientRequest } from '@electron/internal/common/api/net-client-request';

import { app, IncomingMessage, session } from 'electron/main';
import type { ClientRequestConstructorOptions } from 'electron/main';

const { isOnline } = process._linkedBinding('electron_common_net');
const { createConnectionCostMonitor, isConnectionMetered } = process._linkedBinding('electron_browser_connection_cost_monitor');

class Net extends EventEmitter {
  constructor () {
    super();
    // Lazily create the C++ observer on first listener to bridge events
    this.once('newListener', () => {
      const monitor = createConnectionCostMonitor();
      monitor.emit = this.emit.bind(this);
    });
  }

  request (options: ClientRequestConstructorOptions | string, callback?: (message: IncomingMessage) => void) {
    if (!app.isReady()) {
      throw new Error('net module can only be used after app is ready');
    }
    return new ClientRequest(options, callback);
  }

  fetch (input: RequestInfo, init?: RequestInit): Promise<Response> {
    return session.defaultSession.fetch(input, init);
  }

  resolveHost (host: string, options?: Electron.ResolveHostOptions): Promise<Electron.ResolvedHost> {
    return session.defaultSession.resolveHost(host, options);
  }

  isOnline () {
    return isOnline();
  }

  isConnectionMetered () {
    return isConnectionMetered();
  }

  get online () {
    return isOnline();
  }

  get connectionMetered () {
    return isConnectionMetered();
  }
}

module.exports = new Net();
