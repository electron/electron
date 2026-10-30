/* global chrome */
const call = (area, method, ...args) =>
  new Promise((resolve, reject) => {
    area[method](...args, (result) => {
      if (chrome.runtime.lastError) {
        reject(new Error(chrome.runtime.lastError.message));
      } else {
        resolve(result);
      }
    });
  });

const firstSyncChange = new Promise((resolve) => {
  chrome.storage.onChanged.addListener((changes, areaName) => {
    if (areaName === 'sync') resolve(changes);
  });
});

(async () => {
  const { sync, local } = chrome.storage;
  const result = {};
  try {
    await call(sync, 'set', { key: 'sync-value', other: 'sync-other' });
    result.change = await firstSyncChange;
    await call(local, 'set', { key: 'local-value' });

    result.sync = await call(sync, 'get', null);
    result.local = await call(local, 'get', null);

    await call(sync, 'remove', 'other');
    result.syncAfterRemove = await call(sync, 'get', null);

    result.bytesInUse = await call(sync, 'getBytesInUse', null);

    try {
      await call(sync, 'set', { big: 'x'.repeat(sync.QUOTA_BYTES_PER_ITEM) });
      result.perItemError = null;
    } catch (error) {
      result.perItemError = error.message;
    }

    await call(sync, 'clear');
    result.syncAfterClear = await call(sync, 'get', null);
    result.localAfterSyncClear = await call(local, 'get', null);

    result.constants = {
      QUOTA_BYTES: sync.QUOTA_BYTES,
      QUOTA_BYTES_PER_ITEM: sync.QUOTA_BYTES_PER_ITEM,
      MAX_ITEMS: sync.MAX_ITEMS
    };
  } catch (error) {
    result.error = error.message;
  }

  const script = document.createElement('script');
  script.textContent = `require('electron').ipcRenderer.send('storage-sync-result', ${JSON.stringify(JSON.stringify(result))})`;
  document.documentElement.appendChild(script);
})();
