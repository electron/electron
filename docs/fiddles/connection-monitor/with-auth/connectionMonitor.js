const { BrowserWindow } = require('electron');
const { createMachine, send, assign } = require('xstate');
const {
  CM_STATES: STATES,
  CM_EVENTS: EVENTS,
  PING_EVENTS,
  PING_SERVICE_ID,
  CM_IPC_CHANNEL,
  CM_ACTION_TYPES
} = require('./constants.js');
const { createPingMachine } = require('./ping.js');

function authOperationService () {
  return new Promise((resolve, reject) => {
    setTimeout(() => {
      const r = Math.random();
      if (r < 0.5) {
        resolve({
          id: 'xyz57',
          username: 'Clippy'
        });
      } else {
        reject(new Error('Auth flow failed'));
      }
    }, 3000).unref();
  });
}

/**
 * Create a connection monitor machine. The options object is in the form of:
 * ```ts
 * {
 *   url: string,
 *   window: BrowserWindow,
 *   interval?: number,
 * }
 * ```
 * The `url`, `window`, and `interval` are used to instantiate the ping machine
 * the resulting connection monitor service invokes.
 */
function createConnectionMonitorMachine (options) {
  if (typeof options.url !== 'string') {
    throw new TypeError('options.url must be a string');
  }
  if (!options.window) {
    throw new TypeError('options.window must exist');
  }
  // Initialize the ping machine
  const pingMachine = createPingMachine({
    interval: options.interval || 5000,
    url: options.url,
    window: options.window
  });

  // return the connection monitor machine
  return createMachine({
    id: 'connectionMonitor',
    initial: STATES.DISCONNECTED,
    context: { user: undefined },
    /*
     * upon start, invoke the `pingMachine` as a service using the
     * `PING_SERVICE_ID` constant. This machine will forward along
     * `TOGGLE_PING` events to this service at any time. Additionally, this
     * event does **not** cause this machine to change state. The connection
     * monitor will remain in whatever state it is in when the ping service is
     * toggled off.
     */
    invoke: { id: PING_SERVICE_ID, src: pingMachine },
    states: {
      /*
       * Starting in the `DISCONNECTED` state, the machine is looking for
       * two things to happen. First, the `TOGGLE_PING` event which is
       * forwarded along to the ping service. When the ping service establishes
       * a connection it will send back to this machine the `CONNECT` event
       * transitioning the connection monitor to the `CONNECTED` state.
       */
      [STATES.DISCONNECTED]: {
        on: {
          [EVENTS.TOGGLE_PING]: {
            actions: send(
              { type: PING_EVENTS.TOGGLE },
              { to: PING_SERVICE_ID }
            )
          },
          [EVENTS.CONNECT]: {
            target: STATES.CONNECTED
          }
        }
      },
      /*
       * The `CONNECTED` state waits for the ping service to send an
       * `DISCONNECT` event. Additionally, the user can send a `TOGGLE_PING`
       * event at any time resulting in the ping service to turn off; this does
       * **not** cause the connection monitor to transition to `DISCONNECTED`
       */
      [STATES.CONNECTED]: {
        on: {
          [EVENTS.TOGGLE_PING]: {
            actions: send(
              { type: PING_EVENTS.TOGGLE },
              { to: PING_SERVICE_ID }
            )
          },
          [EVENTS.DISCONNECT]: {
            target: STATES.DISCONNECTED
          },
          [EVENTS.TOGGLE_AUTH]: {
            target: STATES.AUTHENTICATING
          }
        }
      },
      [STATES.AUTHENTICATING]: {
        invoke: {
          id: 'authOperationService',
          src: () => authOperationService(),
          onDone: {
            target: STATES.AUTHENTICATED,
            actions: assign({ user: (_, event) => event.data })
          },
          onError: {
            target: STATES.CONNECTED,
            actions: (_, event) => {
              options.window.webContents.send(CM_IPC_CHANNEL, {
                type: CM_ACTION_TYPES.RENDERER.ERROR,
                payload: {
                  error: event.data
                }
              });
            }
          }
        }
      },
      [STATES.AUTHENTICATED]: {
        on: {
          [EVENTS.TOGGLE_PING]: {
            actions: send(
              { type: PING_EVENTS.TOGGLE },
              { to: PING_SERVICE_ID }
            )
          },
          [EVENTS.TOGGLE_AUTH]: {
            target: STATES.CONNECTED
          },
          [EVENTS.DISCONNECT]: {
            target: STATES.DISCONNECTED
          }
        }
      }
    }
  });
}
module.exports = {
  createConnectionMonitorMachine
};
