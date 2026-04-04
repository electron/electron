import { fetchWithSession } from '@electron/internal/browser/api/net-fetch';
import { ClientRequest } from '@electron/internal/common/api/net-client-request';

import type { ClientRequestConstructorOptions, IncomingMessage } from 'electron/utility';

const { isOnline, resolveHost } = process._linkedBinding('electron_common_net');

function request (options: ClientRequestConstructorOptions | string, callback?: (message: IncomingMessage) => void) {
  return new ClientRequest(options, callback);
}

function fetch (input: RequestInfo, init?: RequestInit): Promise<Response> {
  return fetchWithSession(input, init, undefined, request);
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
