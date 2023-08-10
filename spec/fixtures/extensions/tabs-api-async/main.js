/* global chrome */

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  sendResponse(request);
});

const testMap = {
  getZoomSettings () {
    chrome.runtime.sendMessage({ method: 'getZoomSettings' }, response => {
      console.log(JSON.stringify(response));
    });
  },
  setZoomSettings (settings) {
    chrome.runtime.sendMessage({ method: 'setZoomSettings', args: [settings] }, response => {
      console.log(JSON.stringify(response));
    });
  },
  query (params) {
    chrome.runtime.sendMessage({ method: 'query', args: [params] }, response => {
      console.log(JSON.stringify(response));
    });
  },
  getZoom () {
    chrome.runtime.sendMessage({ method: 'getZoom', args: [] }, response => {
      console.log(JSON.stringify(response));
    });
  },
  setZoom (zoom) {
    chrome.runtime.sendMessage({ method: 'setZoom', args: [zoom] }, response => {
      console.log(JSON.stringify(response));
    });
  },
  get () {
    chrome.runtime.sendMessage({ method: 'get' }, response => {
      console.log(JSON.stringify(response));
    });
  },
  reload () {
    chrome.runtime.sendMessage({ method: 'reload' }, response => {
      console.log(JSON.stringify(response));
    });
  },
  update (params) {
    chrome.runtime.sendMessage({ method: 'update', args: [params] }, response => {
      console.log(JSON.stringify(response));
    });
  }
};

const dispatchTest = (event) => {
  const { method, args = [] } = JSON.parse(event.data);
  testMap[method](...args);
};

window.addEventListener('message', dispatchTest, false);
