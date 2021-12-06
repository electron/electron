import { ipcRenderer, contextBridge } from 'electron/renderer';

const policy = window.trustedTypes.createPolicy('electron-default-app', {
  // we trust the SVG contents
  createHTML: input => input
});

async function getOcticonSvg (name: string) {
  try {
    const response = await fetch(`octicon/${name}.svg`);
    const div = document.createElement('div');
    div.innerHTML = policy.createHTML(await response.text());
    return div;
  } catch {
    return null;
  }
}

async function loadSVG (element: HTMLSpanElement) {
  for (const cssClass of element.classList) {
    if (cssClass.startsWith('octicon-')) {
      const icon = await getOcticonSvg(cssClass.substr(8));
      if (icon) {
        for (const elemClass of element.classList) {
          icon.classList.add(elemClass);
        }
        element.before(icon);
        element.remove();
        break;
      }
    }
  }
}

async function initialize () {
  const electronPath = await ipcRenderer.invoke('bootstrap');

  function replaceText (selector: string, text: string) {
    const element = document.querySelector<HTMLElement>(selector);
    if (element) {
      element.innerText = text;
    }
  }

  replaceText('.electron-version', `Electron v${process.versions.electron}`);
  replaceText('.chrome-version', `Chromium v${process.versions.chrome}`);
  replaceText('.node-version', `Node v${process.versions.node}`);
  replaceText('.v8-version', `v8 v${process.versions.v8}`);
  replaceText('.command-example', `${electronPath} path-to-app`);

  for (const element of document.querySelectorAll<HTMLSpanElement>('.octicon')) {
    loadSVG(element);
  }
}

contextBridge.exposeInMainWorld('electronDefaultApp', {
  initialize
});
