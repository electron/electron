/* global chrome */
function testStorageClear (callback) {
  chrome.storage.sync.clear(function () {
    chrome.storage.sync.get(null, function (syncItems) {
      chrome.storage.local.clear(function () {
        chrome.storage.local.get(null, function (localItems) {
          callback(syncItems, localItems);
        });
      });
    });
  });
}

function testStorageRemove (callback) {
  chrome.storage.sync.remove('bar', function () {
    chrome.storage.sync.get({ foo: 'baz' }, function (syncItems) {
      chrome.storage.local.remove(['hello'], function () {
        chrome.storage.local.get(null, function (localItems) {
          callback(syncItems, localItems);
        });
      });
    });
  });
}

function testStorageSet (callback) {
  chrome.storage.sync.set({ foo: 'bar', bar: 'foo' }, function () {
    chrome.storage.sync.get({ foo: 'baz', bar: 'fooo' }, function (syncItems) {
      chrome.storage.local.set({ hello: 'world', world: 'hello' }, function () {
        chrome.storage.local.get(null, function (localItems) {
          callback(syncItems, localItems);
        });
      });
    });
  });
}

function testStorage (callback) {
  testStorageSet(function (syncForSet, localForSet) {
    testStorageRemove(function (syncForRemove, localForRemove) {
      testStorageClear(function (syncForClear, localForClear) {
        callback(
          syncForSet, localForSet,
          syncForRemove, localForRemove,
          syncForClear, localForClear
        );
      });
    });
  });
}

testStorage(function (
  syncForSet, localForSet,
  syncForRemove, localForRemove,
  syncForClear, localForClear
) {
  setTimeout(() => {
    const message = JSON.stringify({
      runtimeId: chrome.runtime.id,
      tabId: chrome.devtools.inspectedWindow.tabId,
      i18nString: chrome.i18n.getMessage('foo', ['bar', 'baz']),
      storageItems: {
        local: {
          set: localForSet,
          remove: localForRemove,
          clear: localForClear
        },
        sync: {
          set: syncForSet,
          remove: syncForRemove,
          clear: syncForClear
        }
      }
    });

    const sendMessage = `require('electron').ipcRenderer.send('answer', ${message})`;
    window.chrome.devtools.inspectedWindow.eval(sendMessage, function () {});
  });
});
