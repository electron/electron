const versionInfoBtn = document.getElementById('version-info')

const electronVersion = process.versions.electron

versionInfoBtn.addEventListener('click', () => {
  const message = `This app is using Electron version: ${electronVersion}`
  document.getElementById('got-version-info').innerHTML = message
})
