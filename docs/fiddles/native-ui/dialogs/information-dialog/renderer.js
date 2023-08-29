const informationBtn = document.getElementById('information-dialog')

informationBtn.addEventListener('click', async () => {
  const index = await window.electronAPI.openInformationDialog()
  const message = `You selected: ${index === 0 ? 'yes' : 'no'}`
  document.getElementById('info-selection').innerHTML = message
})
