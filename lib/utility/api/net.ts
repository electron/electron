import { IncomingMessage } from 'electron/main';
import type { ClientRequestConstructorOptions } from 'electron/main';
import { ClientRequest } from '@electron/internal/common/api/net-client-request';
import { fetchWithSession } from '@electron/internal/browser/api/net-fetch';

export function request (options: ClientRequestConstructorOptions | string, callback?: (message: IncomingMessage) => void) {
  return new ClientRequest(options, callback);
}

export function fetch (input: RequestInfo, init?: RequestInit): Promise<Response> {
  return fetchWithSession(input, init, undefined, request);
}
