const {remote, shell} = require('electron')
const {execFile} = require('child_process')

const {execPath} = remote.process

document.onclick = function (e) {
  e.preventDefault()
  if (e.target.tagName === 'A') {
    shell.openExternal(e.target.href)
  }
  return false
}

document.ondragover = document.ondrop = function (e) {
  e.preventDefault()
  return false
}

const holder = document.getElementById('holder')
holder.ondragover = function () {
  this.className = 'hover'
  return false
}

holder.ondragleave = holder.ondragend = function () {
  this.className = ''
  return false
}

holder.ondrop = function (e) {
  this.className = ''
  e.preventDefault()

  const file = e.dataTransfer.files[0]
  execFile(execPath, [file.path], {
    detached: true, stdio: 'ignore'
  }).unref()
  return false
}

const version = process.versions.electron
document.querySelector('.header-version').innerText = version
document.querySelector('.command-example').innerText = `${execPath} path-to-your-app`
document.querySelector('.quick-start-link').href = `https://github.com/electron/electron/blob/v${version}/docs/tutorial/quick-start.md`
document.querySelector('.docs-link').href = `https://github.com/electron/electron/tree/v${version}/docs#readme`
