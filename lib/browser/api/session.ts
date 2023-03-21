import { fetchWithSession } from '@electron/internal/browser/api/net-fetch';
const { fromPartition, fromPath, Session } = process._linkedBinding('electron_browser_session');

Session.prototype.fetch = function (input: RequestInfo, init?: RequestInit) {
  return fetchWithSession(input, init, this);
};

export default {
  fromPartition,
  fromPath,
  get defaultSession () {
    return fromPartition('');
  }
};
