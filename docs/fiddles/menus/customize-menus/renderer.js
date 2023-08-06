// Tell main process to show the menu when demo button is clicked
const contextMenuBtn = document.getElementById('context-menu')

contextMenuBtn.addEventListener('click', () => {
  window.electronAPI.showContextMenu()
})
