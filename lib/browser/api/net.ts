import { ClientRequest } from '@electron/internal/common/api/net-client-request';

import { app, IncomingMessage, session } from 'electron/main';
import type { ClientRequestConstructorOptions } from 'electron/main';

const { isOnline } = process._linkedBinding('electron_common_net');

function request (options: ClientRequestConstructorOptions | string, callback?: (message: IncomingMessage) => void) {
  if (!app.isReady()) {
    throw new Error('net module can only be used after app is ready');
  }
  return new ClientRequest(options, callback);
}

function fetch (input: RequestInfo, init?: RequestInit): Promise<Response> {
  return session.defaultSession.fetch(input, init);
}

function resolveHost (host: string, options?: Electron.ResolveHostOptions): Promise<Electron.ResolvedHost> {
  return session.defaultSession.resolveHost(host, options);
}

module.exports = {
  request,
  fetch,
  resolveHost,
  isOnline,
  get online () {
    return isOnline();
  }
};
