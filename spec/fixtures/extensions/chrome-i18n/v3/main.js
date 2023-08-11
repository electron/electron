/* global chrome */

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  sendResponse(request);
});

const map = {
  getAcceptLanguages () {
    chrome.i18n.getAcceptLanguages().then((languages) => {
      console.log(JSON.stringify(languages));
    });
  },
  getMessage () {
    const message = chrome.i18n.getMessage('extName');
    console.log(JSON.stringify(message));
  },
  getUILanguage () {
    const language = chrome.i18n.getUILanguage();
    console.log(JSON.stringify(language));
  },
  async detectLanguage (texts) {
    const result = [];
    for (const text of texts) {
      const language = await chrome.i18n.detectLanguage(text);
      result.push(language);
    }
    console.log(JSON.stringify(result));
  }
};

const dispatchTest = (event) => {
  const { method, args = [] } = JSON.parse(event.data);
  map[method](...args);
};

window.addEventListener('message', dispatchTest, false);
