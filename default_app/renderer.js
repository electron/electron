const {remote, shell} = require('electron')
const path = require('path')
const URL = require('url')
const electronPath = path.relative(process.cwd(), remote.process.execPath)

Array.from(document.querySelectorAll('a[href]')).forEach(link => {
  // safely add `?utm_source=default_app
  let url = URL.parse(link.getAttribute('href'), true)
  url.query = Object.assign(url.query, {utm_source: 'default_app'})
  url = URL.format(url)

  link.addEventListener('click', (e) => {
    e.preventDefault()
    shell.openExternal(url)
  })
})

document.querySelector('.electron-version').innerText = `Electron v${process.versions.electron}`
document.querySelector('.chrome-version').innerText = `Chromium v${process.versions.chrome}`
document.querySelector('.node-version').innerText = `Node v${process.versions.node}`
document.querySelector('.v8-version').innerText = `v8 v${process.versions.v8}`
document.querySelector('.command-example').innerText = `${electronPath} path-to-app`
