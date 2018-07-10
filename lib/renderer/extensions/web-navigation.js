const Event = require('./event')
const {ipcRenderer} = require('electron')

class WebNavigation {
  constructor () {
    this.onBeforeNavigate = new Event()
    this.onCompleted = new Event()

    ipcRenderer.on('CHROME_WEBNAVIGATION_ONBEFORENAVIGATE', (event, details) => {
      this.onBeforeNavigate.emit(details)
    })

    ipcRenderer.on('CHROME_WEBNAVIGATION_ONCOMPLETED', (event, details) => {
      this.onCompleted.emit(details)
    })
  }
}

exports.setup = () => {
  return new WebNavigation()
}
