const screenshot = document.getElementById('screen-shot')
const screenshotMsg = document.getElementById('screenshot-path')

screenshot.addEventListener('click', async (event) => {
  screenshotMsg.textContent = 'Gathering screens...'
  screenshotMsg.textContent = await window.electronAPI.takeScreenshot()
})
