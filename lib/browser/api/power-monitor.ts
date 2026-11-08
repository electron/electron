import { EventEmitter } from 'events';

const { createPowerMonitor, getSystemIdleState, getSystemIdleTime, getCurrentThermalState, isOnBatteryPower } =
  process._linkedBinding('electron_browser_power_monitor');

// Hold the native PowerMonitor at module level so it is never garbage-collected
// while this module is alive. The C++ side registers OS-level callbacks (session
// change observer on Windows, shutdown handler on macOS, notification observers)
// that prevent safe collection of the C++ wrapper while those registrations exist.
let pm: any;

class PowerMonitor extends EventEmitter implements Electron.PowerMonitor {
  constructor() {
    super();
    // Don't start the event source until both a) the app is ready and b)
    // there's a listener registered for a powerMonitor event.
    this.once('newListener', () => {
      pm = createPowerMonitor();
      pm.emit = this.emit.bind(this);
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
