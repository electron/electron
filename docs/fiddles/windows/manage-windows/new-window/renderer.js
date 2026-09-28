const newWindowBtn = document.getElementById('new-window')

newWindowBtn.addEventListener('click', () => {
  const url = 'https://electronjs.org'
  window.electronAPI.newWindow({ url, width: 400, height: 320 })
})
