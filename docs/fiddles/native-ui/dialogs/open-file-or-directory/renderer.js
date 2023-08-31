const selectDirBtn = document.getElementById('select-directory')

selectDirBtn.addEventListener('click', async () => {
  const path = await window.electronAPI.openFileDialog()
  document.getElementById('selected-file').innerHTML = `You selected: ${path}`
})
