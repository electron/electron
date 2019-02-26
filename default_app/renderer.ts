import { remote, shell } from 'electron'
import * as fs from 'fs'
import * as path from 'path'
import * as URL from 'url'

function initialize () {
  // Find the shortest path to the electron binary
  const absoluteElectronPath = remote.process.execPath
  const relativeElectronPath = path.relative(process.cwd(), absoluteElectronPath)
  const electronPath = absoluteElectronPath.length < relativeElectronPath.length
    ? absoluteElectronPath
    : relativeElectronPath

  for (const link of document.querySelectorAll<HTMLLinkElement>('a[href]')) {
    // safely add `?utm_source=default_app
    const parsedUrl = URL.parse(link.getAttribute('href')!, true)
    parsedUrl.query = { ...parsedUrl.query, utm_source: 'default_app' }
    const url = URL.format(parsedUrl)

    const openLinkExternally = (e: Event) => {
      e.preventDefault()
      shell.openExternalSync(url)
    }

    link.addEventListener('click', openLinkExternally)
    link.addEventListener('auxclick', openLinkExternally)
  }

  document.querySelector<HTMLAnchorElement>('.electron-version')!.innerText = `Electron v${process.versions.electron}`
  document.querySelector<HTMLAnchorElement>('.chrome-version')!.innerText = `Chromium v${process.versions.chrome}`
  document.querySelector<HTMLAnchorElement>('.node-version')!.innerText = `Node v${process.versions.node}`
  document.querySelector<HTMLAnchorElement>('.v8-version')!.innerText = `v8 v${process.versions.v8}`
  document.querySelector<HTMLAnchorElement>('.command-example')!.innerText = `${electronPath} path-to-app`

  function getOcticonSvg (name: string) {
    const octiconPath = path.resolve(__dirname, 'octicon', `${name}.svg`)
    if (fs.existsSync(octiconPath)) {
      const content = fs.readFileSync(octiconPath, 'utf8')
      const div = document.createElement('div')
      div.innerHTML = content
      return div
    }
    return null
  }

  function loadSVG (element: HTMLSpanElement) {
    for (const cssClass of element.classList) {
      if (cssClass.startsWith('octicon-')) {
        const icon = getOcticonSvg(cssClass.substr(8))
        if (icon) {
          for (const elemClass of element.classList) {
            icon.classList.add(elemClass)
          }
          element.before(icon)
          element.remove()
          break
        }
      }
    }
  }

  for (const element of document.querySelectorAll<HTMLSpanElement>('.octicon')) {
    loadSVG(element)
  }
}

window.addEventListener('load', initialize)
