import Event from '@electron/internal/renderer/extensions/event'
const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')

class WebNavigation {
  private onBeforeNavigate = new Event()
  private onCompleted = new Event()

  constructor () {
    ipcRendererInternal.on('CHROME_WEBNAVIGATION_ONBEFORENAVIGATE', (event: Event, details: any) => {
      this.onBeforeNavigate.emit(details)
    })

    ipcRendererInternal.on('CHROME_WEBNAVIGATION_ONCOMPLETED', (event: Event, details: any) => {
      this.onCompleted.emit(details)
    })
  }
}

exports.setup = () => {
  return new WebNavigation()
}
