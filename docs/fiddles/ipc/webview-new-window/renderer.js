const webview = document.getElementById('webview')
webview.addEventListener('new-window', () => {
  console.log('got new-window event')
})
