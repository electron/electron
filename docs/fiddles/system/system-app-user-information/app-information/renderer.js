const appInfoBtn = document.getElementById('app-info')

appInfoBtn.addEventListener('click', async () => {
  const path = await window.electronAPI.getAppPath()
  const message = `This app is located at: ${path}`
  document.getElementById('got-app-info').innerHTML = message
})
