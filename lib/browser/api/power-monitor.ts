import { EventEmitter } from 'events';

const { createPowerMonitor, getSystemIdleState, getSystemIdleTime, getCurrentThermalState, isOnBatteryPower } =
  process._linkedBinding('electron_browser_power_monitor');

// Hold the native PowerMonitor at module level so it is never garbage-collected
// while this module is alive. The C++ side registers OS-level callbacks (HWND
// user-data on Windows, shutdown handler on macOS, notification observers) that
// prevent safe collection of the C++ wrapper while those registrations exist.
let pm: any;

class PowerMonitor extends EventEmitter implements Electron.PowerMonitor {
  constructor() {
    super();
    // Don't start the event source until both a) the app is ready and b)
    // there's a listener registered for a powerMonitor event.
    this.once('newListener', () => {
      pm = createPowerMonitor();
      pm.emit = this.emit.bind(this);

      if (process.platform === 'linux') {
        // On Linux, we inhibit shutdown in order to give the app a chance to
        // decide whether or not it wants to prevent the shutdown. We don't
        // inhibit the shutdown event unless there's a listener for it. This
        // keeps the C++ code informed about whether there are any listeners.
        pm.setListeningForShutdown(this.listenerCount('shutdown') > 0);
        this.on('newListener', (event) => {
          if (event === 'shutdown') {
            pm.setListeningForShutdown(this.listenerCount('shutdown') + 1 > 0);
          }
        });
        this.on('removeListener', (event) => {
          if (event === 'shutdown') {
            pm.setListeningForShutdown(this.listenerCount('shutdown') > 0);
          }
        });
      }
    });
  }

  getSystemIdleState(idleThreshold: number) {
    return getSystemIdleState(idleThreshold);
  }

  getCurrentThermalState() {
    return getCurrentThermalState();
  }

  getSystemIdleTime() {
    return getSystemIdleTime();
  }

  isOnBatteryPower() {
    return isOnBatteryPower();
  }

  get onBatteryPower() {
    return this.isOnBatteryPower();
  }
}

module.exports = new PowerMonitor();
