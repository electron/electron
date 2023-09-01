const manageWindowBtn = document.getElementById('manage-window')

window.electronAPI.onBoundsChanged((event, bounds) => {
  const manageWindowReply = document.getElementById('manage-window-reply')
  const message = `Size: ${bounds.size} Position: ${bounds.position}`
  manageWindowReply.textContent = message
})

manageWindowBtn.addEventListener('click', (event) => {
  window.electronAPI.createDemoWindow()
})
