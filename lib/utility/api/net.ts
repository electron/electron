import { IncomingMessage } from 'electron/main';
import type { ClientRequestConstructorOptions } from 'electron/main';
import { ClientRequest } from '@electron/internal/common/api/net-client-request';

export function request (options: ClientRequestConstructorOptions | string, callback?: (message: IncomingMessage) => void) {
  return new ClientRequest(options, callback);
}
