const os = require('os')
const homeDir = os.homedir()

const sysInfoBtn = document.getElementById('system-info')

sysInfoBtn.addEventListener('click', () => {
  const message = `Your system home directory is: ${homeDir}`
  document.getElementById('got-system-info').innerHTML = message
})