const newWindowBtn = document.getElementById('frameless-window')

newWindowBtn.addEventListener('click', () => {
  const url = 'data:text/html,<h2>Hello World!</h2><a id="close" href="javascript:window.close()">Close this Window</a>'
  window.electronAPI.createFramelessWindow({ url })
})
