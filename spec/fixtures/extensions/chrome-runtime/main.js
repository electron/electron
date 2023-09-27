/* global chrome */

function evalInMainWorld (fn) {
  const script = document.createElement('script');
  script.textContent = `((${fn})())`;
  document.documentElement.appendChild(script);
}

async function exec (name) {
  let result;
  switch (name) {
    case 'getManifest':
      result = chrome.runtime.getManifest();
      break;
    case 'id':
      result = chrome.runtime.id;
      break;
    case 'getURL':
      result = chrome.runtime.getURL('main.js');
      break;
    case 'getPlatformInfo': {
      result = await new Promise(resolve => {
        chrome.runtime.sendMessage(name, resolve);
      });
      break;
    }
  }

  const funcStr = `() => { require('electron').ipcRenderer.send('success', ${JSON.stringify(result)}) }`;
  evalInMainWorld(funcStr);
}

window.addEventListener('message', event => {
  exec(event.data.name);
});

evalInMainWorld(() => {
  window.exec = name => window.postMessage({ name });
});
