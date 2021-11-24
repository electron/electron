function onlineStatusIndicator () {
  document.getElementById('status').innerHTML = navigator.onLine ? 'online' : 'offline'
}

window.addEventListener('online', onlineStatusIndicator)
window.addEventListener('offline', onlineStatusIndicator)

onlineStatusIndicator()
