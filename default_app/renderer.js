const {remote, shell} = require('electron')
const {execPath} = remote.process

document.onclick = (e) => {
  e.preventDefault()
  if (e.target.tagName === 'A') {
    shell.openExternal(e.target.href)
  }
  return false
}

const version = process.versions.electron
document.querySelector('.header-version').innerText = version
document.querySelector('.command-example').innerText = `${execPath} path-to-your-app`
document.querySelector('.quick-start-link').href = `https://github.com/electron/electron/blob/v${version}/docs/tutorial/quick-start.md`
document.querySelector('.docs-link').href = `https://github.com/electron/electron/tree/v${version}/docs#readme`
