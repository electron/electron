import { IncomingMessage, session } from 'electron/main';
import type { ClientRequestConstructorOptions } from 'electron/main';
import { ClientRequest } from '@electron/internal/browser/api/net-client-request';

const { isOnline } = process._linkedBinding('electron_browser_net');

export function request (options: ClientRequestConstructorOptions | string, callback?: (message: IncomingMessage) => void) {
  return new ClientRequest(options, callback);
}

export function fetch (input: RequestInfo, init?: RequestInit): Promise<Response> {
  return session.defaultSession.fetch(input, init);
}

export function resolveHost (host: string, options?: Electron.ResolveHostOptions): Promise<Electron.ResolvedHost> {
  return session.defaultSession.resolveHost(host, options);
}

exports.isOnline = isOnline;

Object.defineProperty(exports, 'online', {
  get: () => isOnline()
});
