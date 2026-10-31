const { powerMonitor } = process._linkedBinding('electron_browser_power_monitor');
const emitter = powerMonitor as NodeJS.EventEmitter;

if (process.platform === 'linux') {
  // On Linux, we inhibit shutdown in order to give the app a chance to
  // decide whether or not it wants to prevent the shutdown. We don't
  // inhibit the shutdown event unless there's a listener for it. This
  // keeps the C++ code informed about whether there are any listeners.
  emitter.on('newListener', (event: string | symbol) => {
    if (event === 'shutdown') {
      powerMonitor._setListeningForShutdown(true);
    }
  });
  emitter.on('removeListener', (event: string | symbol) => {
    if (event === 'shutdown') {
      powerMonitor._setListeningForShutdown(powerMonitor.listenerCount('shutdown') > 0);
    }
  });
}

// The system is only observed once there is a listener for a powerMonitor
// event. (Registered last: adding a listener emits 'newListener' itself.)
emitter.once('newListener', () => powerMonitor._start());

export default powerMonitor;
