/* eslint-disable */

function evalInMainWorld(fn) {
  const script = document.createElement('script')
  script.textContent = `((${fn})())`
  document.documentElement.appendChild(script)
}

async function exec(name) {
  let result
  switch (name) {
    case 'getMessage':
      result = {
        id: chrome.i18n.getMessage('@@extension_id'),
        name: chrome.i18n.getMessage('extName'),
      }
      break
    case 'getAcceptLanguages':
      result = await new Promise(resolve => chrome.i18n.getAcceptLanguages(resolve))
      break
  }

  const funcStr = `() => { require('electron').ipcRenderer.send('success', ${JSON.stringify(result)}) }`
  evalInMainWorld(funcStr)
}

window.addEventListener('message', event => {
  exec(event.data.name)
})

evalInMainWorld(() => {
  window.exec = name => window.postMessage({ name })
})
