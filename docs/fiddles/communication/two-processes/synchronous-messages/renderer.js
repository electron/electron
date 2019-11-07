const { ipcRenderer } = require('electron')

const syncMsgBtn = document.getElementById('sync-msg')

syncMsgBtn.addEventListener('click', () => {
    const reply = ipcRenderer.sendSync('synchronous-message', 'ping')
    const message = `Synchronous message reply: ${reply}`
    document.getElementById('sync-reply').innerHTML = message
})