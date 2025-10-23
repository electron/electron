const { ipcRenderer, contextBridge } = require('electron/renderer');

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
  function replaceText (selector: string, text: string, link?: string) {
    const element = document.querySelector<HTMLElement>(selector);
    if (element) {
      if (link) {
        const anchor = document.createElement('a');
        anchor.textContent = text;
        anchor.href = link;
        anchor.target = '_blank';
        element.appendChild(anchor);
      } else {
        element.innerText = text;
      }
    }
  }

  replaceText('.electron-version', `Electron v${process.versions.electron}`, 'https://electronjs.org/docs');
  replaceText('.chrome-version', `Chromium v${process.versions.chrome}`, 'https://developer.chrome.com/docs/chromium');
  replaceText('.node-version', `Node v${process.versions.node}`, `https://nodejs.org/docs/v${process.versions.node}/api`);
  replaceText('.v8-version', `v8 v${process.versions.v8}`, 'https://v8.dev/docs');
  replaceText('.command-example', `${electronPath} path-to-app`);

  for (const element of document.querySelectorAll<HTMLSpanElement>('.octicon')) {
    loadSVG(element);
  }
}

contextBridge.exposeInMainWorld('electronDefaultApp', {
  initialize
});
