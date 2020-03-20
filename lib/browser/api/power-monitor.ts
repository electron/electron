import { EventEmitter } from 'events';
import { app } from 'electron';

const { createPowerMonitor, getSystemIdleState, getSystemIdleTime } = process.electronBinding('power_monitor');

class PowerMonitor extends EventEmitter {
  constructor () {
    super();
    // Don't start the event source until both a) the app is ready and b)
    // there's a listener registered for a powerMonitor event.
    this.once('newListener', () => {
      app.whenReady().then(() => {
        const pm = createPowerMonitor();
        pm.emit = this.emit.bind(this);

        if (process.platform === 'linux') {
          const updateShutdownListeningState = () => {
            pm.setListeningForShutdown(this.listenerCount('shutdown') > 0);
          };
          this.on('newListener', updateShutdownListeningState);
          this.on('removeListener', updateShutdownListeningState);
        }
      });
    });
  }

  getSystemIdleState (idleThreshold: number) {
    return getSystemIdleState(idleThreshold);
  }

  getSystemIdleTime () {
    return getSystemIdleTime();
  }
}

module.exports = new PowerMonitor();
