import { ipcRenderer } from 'electron'

function initialize () {
  const electronPath = ipcRenderer.sendSync('bootstrap')

  function replaceText (selector: string, text: string) {
    const element = document.querySelector<HTMLElement>(selector)
    if (element) {
      element.innerText = text
    }
  }

  replaceText('.electron-version', `Electron v${process.versions.electron}`)
  replaceText('.chrome-version', `Chromium v${process.versions.chrome}`)
  replaceText('.node-version', `Node v${process.versions.node}`)
  replaceText('.v8-version', `v8 v${process.versions.v8}`)
  replaceText('.command-example', `${electronPath} path-to-app`)
}

document.addEventListener('DOMContentLoaded', initialize)
