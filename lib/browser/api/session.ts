import { fetchWithSession } from '@electron/internal/browser/api/net-fetch';
const { fromPartition, Session } = process._linkedBinding('electron_browser_session');

Session.prototype.fetch = function (input: RequestInfo, init?: RequestInit) {
  return fetchWithSession(input, init, this);
};

export default {
  fromPartition,
  get defaultSession () {
    return fromPartition('');
  }
};
