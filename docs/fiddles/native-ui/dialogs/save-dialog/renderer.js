const saveBtn = document.getElementById('save-dialog')

saveBtn.addEventListener('click', async () => {
  const path = await window.electronAPI.saveDialog()
  document.getElementById('file-saved').innerHTML = `Path selected: ${path}`
})
