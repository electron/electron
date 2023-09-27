const listenToWindowBtn = document.getElementById('listen-to-window')
const focusModalBtn = document.getElementById('focus-on-modal-window')

const hideFocusBtn = () => {
  focusModalBtn.classList.add('disappear')
  focusModalBtn.classList.remove('smooth-appear')
  focusModalBtn.removeEventListener('click', focusWindow)
}

const showFocusBtn = (btn) => {
  focusModalBtn.classList.add('smooth-appear')
  focusModalBtn.classList.remove('disappear')
  focusModalBtn.addEventListener('click', focusWindow)
}
const focusWindow = () => {
  window.electronAPI.focusDemoWindow()
}

window.electronAPI.onWindowFocus(hideFocusBtn)
window.electronAPI.onWindowClose(hideFocusBtn)
window.electronAPI.onWindowBlur(showFocusBtn)

listenToWindowBtn.addEventListener('click', () => {
  window.electronAPI.showDemoWindow()
})
