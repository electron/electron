const { contextBridge, ipcRenderer } = require('electron');
const {
  CM_IPC_CHANNEL,
  PING_STATES,
  CM_EVENTS,
  CM_STATES,
  CM_ACTION_TYPES
} = require('./constants.js');

/*
 * Using the secure contextBridge, create a `mainAPI` that the
 * renderer process can use to securely communicate with the main
 * process. This is available on the `window` object on the renderer
 * process, i.e. `window.mainAPI.requestState()`
 */
contextBridge.exposeInMainWorld('mainAPI', {
  /**
   * Get the current state of the connection monitor instance. Useful
   * for when the app first starts and the renderer process needs to
   * know what the current state of the main process is.
   */
  requestState: () =>
    ipcRenderer.invoke(CM_IPC_CHANNEL, {
      type: CM_ACTION_TYPES.MAIN.REQUEST_STATE
    }),
  /**
   * Toggle the ping operation. The ping service instance is either
   * on or off; this method will switch it between the respective states.
   * Turning off the ping service will not change the state of the connection
   * monitor.
   */
  togglePing: () =>
    ipcRenderer.invoke(CM_IPC_CHANNEL, {
      type: CM_ACTION_TYPES.MAIN.TRIGGER_EVENT,
      payload: CM_EVENTS.TOGGLE_PING
    }),
  /**
   * The renderer process uses this method to listen for state transition
   * changes of the main process services. The data passed to the listener will
   * be in the form:
   * ```ts
   * {
   *   type: string,
   *   payload: {
   *     connectionMonitor?: { state: string },
   *     ping?: { state: string }
   *   }
   * }
   * ```
   */
  addConnectionMonitorListener: (listener) => {
    ipcRenderer.on(CM_IPC_CHANNEL, listener);
    return () => {
      ipcRenderer.removeListener(CM_IPC_CHANNEL, listener);
    };
  },
  /**
   * These constants are all made available for the renderer to use as necessary.
   * In a more robust application these would most likely be bundled directly
   * with the renderer code.
   */
  constants: {
    CM_STATES,
    PING_STATES,
    CM_ACTION_TYPES
  },
  /**
   * Toggle the authentication operation
   */
  toggleAuth: () =>
    ipcRenderer.invoke(CM_IPC_CHANNEL, {
      type: CM_ACTION_TYPES.MAIN.TRIGGER_EVENT,
      payload: CM_EVENTS.TOGGLE_AUTH
    })
});
