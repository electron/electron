import { EventEmitter } from 'events'
import { app } from 'electron'

const { createPowerMonitor, getSystemIdleState, getSystemIdleTime, blockShutdown, unblockShutdown } = process.electronBinding('power_monitor')

class PowerMonitor extends EventEmitter {
  constructor () {
    super()
    // Don't start the event source until both a) the app is ready and b)
    // there's a listener registered for a powerMonitor event.
    this.once('newListener', () => {
      app.whenReady().then(() => {
        const pm = createPowerMonitor()
        pm.emit = this.emit.bind(this)
      })
    })

    if (process.platform === 'linux') {
      // In order to delay system shutdown when e.preventDefault() is invoked
      // on a powerMonitor 'shutdown' event, we need an org.freedesktop.login1
      // shutdown delay lock. For more details see the "Taking Delay Locks"
      // section of https://www.freedesktop.org/wiki/Software/systemd/inhibit/
      //
      // So here we watch for 'shutdown' listeners to be added or removed and
      // set or unset our shutdown delay lock accordingly.
      this.on('newListener', (event) => {
        // whenever the listener count is incremented to one...
        if (event === 'shutdown' && this.listenerCount('shutdown') === 0) {
          blockShutdown()
        }
      })
      this.on('removeListener', (event) => {
        // whenever the listener count is decremented to zero...
        if (event === 'shutdown' && this.listenerCount('shutdown') === 0) {
          unblockShutdown()
        }
      })
    }
  }

  getSystemIdleState (idleThreshold: number) {
    return getSystemIdleState(idleThreshold)
  }

  getSystemIdleTime () {
    return getSystemIdleTime()
  }
}

module.exports = new PowerMonitor()
