/* global chrome */

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  sendResponse(request);
});

const testMap = {
  isEnabled () {
    chrome.runtime.sendMessage({ method: 'isEnabled' }, response => {
      console.log(JSON.stringify(response));
    });
  },
  setIcon () {
    chrome.runtime.sendMessage({ method: 'setIcon' }, response => {
      console.log(JSON.stringify(response));
    });
  },
  getBadgeText () {
    chrome.runtime.sendMessage({ method: 'getBadgeText' }, response => {
      console.log(JSON.stringify(response));
    });
  }
};

const dispatchTest = (event) => {
  const { method, args = [] } = JSON.parse(event.data);
  testMap[method](...args);
};

window.addEventListener('message', dispatchTest, false);
