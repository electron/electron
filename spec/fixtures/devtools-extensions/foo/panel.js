/* global chrome */
function testStorageClear(callback) {
  chrome.storage.local.clear(function () {
    chrome.storage.local.get(null, callback);
  });
}

function testStorageRemove(callback) {
  chrome.storage.local.remove(['hello'], function () {
    chrome.storage.local.get(null, callback);
  });
}

function testStorageSet(callback) {
  chrome.storage.local.set({ hello: 'world', world: 'hello' }, function () {
    chrome.storage.local.get(null, callback);
  });
}

function testStorage(callback) {
  testStorageSet(function (set) {
    testStorageRemove(function (remove) {
      testStorageClear(function (clear) {
        callback({ set, remove, clear });
      });
    });
  });
}

testStorage(function (local) {
  setTimeout(() => {
    const message = JSON.stringify({
      runtimeId: chrome.runtime.id,
      tabId: chrome.devtools.inspectedWindow.tabId,
      i18nString: chrome.i18n.getMessage('foo', ['bar', 'baz']),
      storageItems: { local }
    });

    const sendMessage = `require('electron').ipcRenderer.send('answer', ${message})`;
    window.chrome.devtools.inspectedWindow.eval(sendMessage, function () {});
  });
});
