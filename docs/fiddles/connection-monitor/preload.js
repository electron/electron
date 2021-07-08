const { contextBridge, ipcRenderer } = require('electron');
const {
  CONNECTION_MONITOR_IPC_CHANNEL,
  PING_STATES,
  CONNECTION_MONITOR_EVENTS,
  CONNECTION_MONITOR_STATES,
  CONNECTION_MONITOR_IPC_REDUCER_ACTION_TYPES: ACTION_TYPES
} = require('./constants.js');

/*
 * Using the secure contextBridge, create a "mainAPI" that the
 * renderer process can use to securely communicate with the main
 * process. This is available on the `window` object on the renderer
 * process, i.e. `window.mainAPI.requestInitState()`
 */
contextBridge.exposeInMainWorld('mainAPI', {
  /**
   * Get the current state of the ConnectionMonitor instance. Useful
   * for when the app first starts and the renderer process needs to
   * know what the current state of the main process is.
   */
  requestState: () =>
    ipcRenderer.invoke(CONNECTION_MONITOR_IPC_CHANNEL, {
      type: ACTION_TYPES.MAIN.REQUEST_STATE
    }),
  /**
   * Toggle the ping operation. The underlying ping machine is either
   * on or off; this method will switch it between the respective states.
   * Turning off the ping machine will not change the state of the connection
   * monitor.
   */
  togglePing: () =>
    ipcRenderer.invoke(CONNECTION_MONITOR_IPC_CHANNEL, {
      type: ACTION_TYPES.MAIN.TRIGGER_EVENT,
      payload: CONNECTION_MONITOR_EVENTS.TOGGLE_PING
    }),
  /**
   * Toggles the authentication state of the connection monitor. This methods
   * switches between logging in and logging out the user depending on the
   * relative state.
   */
  toggleAuth: () =>
    ipcRenderer.invoke(CONNECTION_MONITOR_IPC_CHANNEL, {
      type: ACTION_TYPES.MAIN.TRIGGER_EVENT,
      payload: CONNECTION_MONITOR_EVENTS.TOGGLE_AUTH
    }),
  /**
   * The renderer process can use this method to listen for state transition
   * changes of the connection monitor. The data passed to the listener will
   * be in the form:
   * ```ts
   * {
   *   type: string,
   *   payload: {
   *     connectionMonitor?: { state: string, context: any },
   *     ping?: { state: string }
   *   }
   * }
   * ```
   */
  addConnectionMonitorListener: (listener) => {
    ipcRenderer.on(CONNECTION_MONITOR_IPC_CHANNEL, listener);
    return () => {
      ipcRenderer.removeListener(CONNECTION_MONITOR_IPC_CHANNEL, listener);
    };
  },
  /**
   * These constants are all made available for the renderer to use as necessary.
   * In a more robust application these would most likely be bundled directly
   * with the renderer code.
   */
  constants: {
    CONNECTION_MONITOR_STATES,
    PING_STATES,
    CONNECTION_MONITOR_IPC_REDUCER_ACTION_TYPES: ACTION_TYPES
  }
});
