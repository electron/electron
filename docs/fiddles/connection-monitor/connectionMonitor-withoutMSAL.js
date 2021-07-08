const { createMachine, send, assign } = require('xstate');
const { BrowserWindow } = require('electron');
const {
  CONNECTION_MONITOR_IPC_CHANNEL,
  CONNECTION_MONITOR_STATES: STATES,
  CONNECTION_MONITOR_EVENTS: EVENTS,
  PING_EVENTS,
  PING_SERVICE_ID,
  CONNECTION_MONITOR_IPC_REDUCER_ACTION_TYPES: ACTION_TYPES
} = require('./constants.js');
const { createPingMachine } = require('./ping.js');

/**
 * This method is the core to the authentication part of the connection
 * monitor. Like the pingOperationService, this method returns a `Promise`
 * that relates `resolve` to "authenticated" and `reject` to "failed". As
 * described in the machine comments, this example transitions the machine
 * back to the **CONNECTED** state when authentication fails and forwards
 * along the error to the renderer process. A more complex application may
 * have an **ERROR** state that requires further user interaction.
 */
function authenticationOperationService () {
  return new Promise((resolve, reject) => {
    if (Math.random() < 0.5) {
      resolve();
    } else {
      reject(new Error('Failed to authenticate'));
    }
  });
}

/**
 * Create a connection monitor machine.
 */
function createConnectionMonitorMachine (options) {
  // Initialize the ping machine
  const pingMachine = createPingMachine({
    interval: options.interval || 5000,
    url: 'https://login.microsoftonline.com'
  });

  // return the connection monitor machine
  return createMachine({
    id: 'connectionMonitor',
    context: { user: null },
    initial: STATES.DISCONNECTED,
    /*
     * upon creation, invoke the pingMachine as a service using the
     * `PING_SERVICE_ID` constant. This machine will forward along
     * **TOGGLE_PING** events to this service at any time. Additionally, this
     * event does **not** cause this machine to change state. The connection
     * monitor will remain in whatever state it is in when the ping service is
     * toggled off.
     */
    invoke: { id: PING_SERVICE_ID, src: pingMachine },
    states: {
      /*
       * Starting in the **DISCONNECTED** state, the machine is looking for
       * two things to happen. First, the **TOGGLE_PING** event which is
       * forwarded along to the Ping service. When the ping service establishes
       * a connection it will send back to this machine the **CONNECT** event
       * transitioning the Connection Monitor to the **CONNECTED** state.
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
       * The **CONNECTED** state waits for either the Ping service to send an
       * **DISCONNET** event or the user to send a **TOGGLE_AUTH** event
       * resulting in a transition to the **DISCONNECTED** and **AUTHENTICATING**
       * states respectfully. Additionally, the user can send a **TOGGLE_PING**
       * event at any time resulting in the Ping machine to turn off; this does
       * **not** cause the Connection Monitor to transition to **DISCONNECTED**
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
      /*
       * The **AUTHENTICATING** state invokes the
       * `authenticationOperationService` method similar to how the Ping
       * machine's **PINGING** state invokes the `pingOperationService`. The
       * service is expected to return a promise that resolves for a successful
       * authentication or reject for a failed auth flow. When the auth flow
       * succeeds, the result is assign to the machine's context and the
       * machine transitions to the **AUTHENTICATED** state. If the flow fails,
       * the error is sent to the renderer process and the machine transitions
       * back to the **CONNECTED** state.
       */
      [STATES.AUTHENTICATING]: {
        invoke: {
          id: 'authenticateService',
          src: () =>
            authenticationOperationService(),
          onDone: {
            target: STATES.AUTHENTICATED,
            actions: assign({ user: (_, event) => event.data })
          },
          onError: {
            target: STATES.CONNECTED,
            actions: (_, event) => {
              options.window.webContents.send(CONNECTION_MONITOR_IPC_CHANNEL, {
                type: ACTION_TYPES.RENDERER.ERROR,
                payload: {
                  error: event.data
                }
              });
            }
          }
        }
      },
      /*
       * Once authenticated, the user can log out resulting in a transition
       * back to **CONNECTED** state. If the Ping service fails to maintain a
       * connection, the **DISCONNECT** event will transition the use back to
       * the **DISCONNECTED** state, but will not clear the user from context.
       * Ideally, when the App reconnects, the service can reauthenticate the
       * user silently. This example does not implement this flow, but it is
       * possible using the MSAL library.
       */
      [STATES.AUTHENTICATED]: {
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
            target: STATES.CONNECTED,
            actions: assign({ user: null })
          }
        }
      }
    }
  });
}
module.exports = {
  createConnectionMonitorMachine
};
